/*
 *  itscam_http_transport.cpp
 *
 *  ITSCAM Client SDK - Shared HTTP transport implementation
 *
 *  Copyright (c) 2026 Pumatronix
 */

//=========================================================================
// Platform configuration (must be before any includes)
//=========================================================================

#if defined(_WIN32) || defined(__MINGW32__)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0A00
    #endif
#endif

#include "itscam_http_transport.h"
#include "../itscam_os.h"

#include <stdarg.h>
#include <stdio.h>

#include <atomic>

// cpp-httplib (header-only).  When the SDK is compiled with TLS support,
// CPPHTTPLIB_MBEDTLS_SUPPORT is defined externally by the build system so
// that httplib pulls in the mbedTLS backend.
#include "../3rdparty/httplib.h"

namespace itscam {
namespace detail {

using Mutex = itscam_os::Mutex;
template<typename M> using LockGuard = itscam_os::LockGuard<M>;

//=========================================================================
// Impl
//=========================================================================

struct HttpTransport::Impl {

    // --- Connection target ---------------------------------------------------

    std::string host;
    uint16_t    port       = 0;
    std::string scheme     = "http";
    bool        configured = false;

    // --- Auth ---------------------------------------------------------------

    AuthConfig auth;

    // --- TLS ----------------------------------------------------------------

    TlsConfig tls;

    // --- Logging ------------------------------------------------------------

    std::function<void(LogLevel, const std::string&)> logHandler;
    mutable Mutex logMtx;

    // --- Active client (for streaming cancel) ------------------------------

    Mutex               clientMtx;
    httplib::Client*    activeClient = nullptr;
    std::atomic<bool>   cancelRequested{false};

    // --- Helpers ------------------------------------------------------------

    void logVa(LogLevel lvl, const char* fmt, va_list ap) const {
        LockGuard<Mutex> lk(logMtx);
        if (!logHandler) return;
        char buf[1024];
        vsnprintf(buf, sizeof(buf), fmt, ap);
        logHandler(lvl, buf);
    }

    void logInfo(const char* fmt, ...) const {
        va_list ap;
        va_start(ap, fmt);
        logVa(LogLevel::Info, fmt, ap);
        va_end(ap);
    }

    void logError(const char* fmt, ...) const {
        va_list ap;
        va_start(ap, fmt);
        logVa(LogLevel::Error, fmt, ap);
        va_end(ap);
    }

    /// Construct a cpp-httplib client (HTTP or HTTPS) configured with all
    /// the timeouts, auth headers and TLS settings.  Caller owns the
    /// returned object.
    ///
    /// We use the universal `httplib::Client(scheme_host_port)`
    /// constructor so the same `httplib::Client` instance transparently
    /// dispatches to `httplib::ClientImpl` (HTTP) or `httplib::SSLClient`
    /// (HTTPS, when CPPHTTPLIB_*_SUPPORT is enabled) under the hood.
    std::unique_ptr<httplib::Client> makeClient(uint32_t timeoutMs) {
        uint16_t actualPort = port;
        if (actualPort == 0) {
            actualPort = isHttps() ? 443 : 80;
        }

        std::unique_ptr<httplib::Client> cli;
        if (isHttps()) {
#if defined(CPPHTTPLIB_OPENSSL_SUPPORT) || \
    defined(CPPHTTPLIB_MBEDTLS_SUPPORT)
            const std::string url = "https://" + host + ":" +
                                    std::to_string(actualPort);
            if (!tls.clientCertPem.empty() && !tls.clientKeyPem.empty()) {
                cli.reset(new httplib::Client(url, tls.clientCertPem,
                                              tls.clientKeyPem));
            } else {
                cli.reset(new httplib::Client(url));
            }
            applyTls(cli.get());
#else
            // No TLS backend compiled in; fall back to plain HTTP so the
            // connection fails loudly inside processError().
            cli.reset(new httplib::Client(host, actualPort));
#endif
        } else {
            cli.reset(new httplib::Client(host, actualPort));
        }

        cli->set_connection_timeout(std::chrono::milliseconds(timeoutMs));
        cli->set_read_timeout(std::chrono::milliseconds(timeoutMs));
        cli->set_write_timeout(std::chrono::milliseconds(timeoutMs));
        cli->set_keep_alive(false);
        cli->set_follow_location(false);

        if (!auth.bearerToken.empty()) {
            cli->set_bearer_token_auth(auth.bearerToken);
        } else if (!auth.basicUser.empty()) {
            cli->set_basic_auth(auth.basicUser, auth.basicPassword);
        }

        return cli;
    }

    bool isHttps() const {
        return scheme == "https" || scheme == "HTTPS";
    }

    void applyTls(httplib::Client* cli) {
        if (!cli) return;
#if defined(CPPHTTPLIB_OPENSSL_SUPPORT) || defined(CPPHTTPLIB_MBEDTLS_SUPPORT)
        cli->enable_server_certificate_verification(tls.verifyServerCert);
        if (!tls.caCertFile.empty()) {
            cli->set_ca_cert_path(tls.caCertFile);
        }
#else
        (void)cli;
#endif
    }

    /// Convert a httplib::Response (success path) into HttpResponse.
    HttpResponse buildResponse(const httplib::Response& res) {
        HttpResponse out;
        out.status = res.status;
        auto ctIt = res.headers.find("Content-Type");
        if (ctIt != res.headers.end()) {
            out.contentType = ctIt->second;
        }
        for (const auto& kv : res.headers) {
            out.headers[kv.first] = kv.second;
        }
        out.body.assign(res.body.begin(), res.body.end());
        return out;
    }

    /// Map an httplib::Result onto either a successful HttpResponse or a
    /// transport-level Error.
    Result<HttpResponse> processError(httplib::Result& res,
                                      const std::string& method,
                                      const std::string& path) {
        if (!res) {
            auto err = res.error();
            std::string label;
            switch (err) {
                case httplib::Error::ConnectionTimeout:
                case httplib::Error::Read:
                    logError("%s %s: timeout", method.c_str(), path.c_str());
                    return Error{Error::Timeout,
                                 method + " " + path + ": request timed out"};
                case httplib::Error::Connection:
                case httplib::Error::BindIPAddress:
                    logError("%s %s: connection failed", method.c_str(),
                             path.c_str());
                    return Error{Error::ConnectionFailed,
                                 method + " " + path + ": connection failed"};
                case httplib::Error::SSLConnection:
                case httplib::Error::SSLLoadingCerts:
                case httplib::Error::SSLServerVerification:
                case httplib::Error::SSLServerHostnameVerification:
                    logError("%s %s: TLS error %d", method.c_str(),
                             path.c_str(), static_cast<int>(err));
                    return Error{Error::ConnectionFailed,
                                 method + " " + path + ": TLS handshake failed"};
                case httplib::Error::Canceled:
                    return Error{Error::Disconnected,
                                 method + " " + path + ": cancelled"};
                default:
                    logError("%s %s: httplib error %d", method.c_str(),
                             path.c_str(), static_cast<int>(err));
                    return Error{Error::ConnectionFailed,
                                 method + " " + path + ": request failed"};
            }
        }
        return buildResponse(*res);
    }
};

//=========================================================================
// Constructor / destructor / move
//=========================================================================

HttpTransport::HttpTransport() : mImpl(new Impl()) {}
HttpTransport::~HttpTransport() = default;

HttpTransport::HttpTransport(HttpTransport&&) noexcept = default;
HttpTransport& HttpTransport::operator=(HttpTransport&&) noexcept = default;

//=========================================================================
// Configuration
//=========================================================================

void HttpTransport::setBaseUrl(const std::string& host, uint16_t port,
                               const std::string& scheme) {
    mImpl->host       = host;
    mImpl->port       = port;
    mImpl->scheme     = scheme.empty() ? std::string("http") : scheme;
    mImpl->configured = !host.empty();
    mImpl->logInfo("Base URL set to %s://%s:%u",
                   mImpl->scheme.c_str(), host.c_str(),
                   static_cast<unsigned>(
                       port != 0 ? port : (mImpl->isHttps() ? 443 : 80)));
}

const std::string& HttpTransport::host()   const { return mImpl->host; }
uint16_t           HttpTransport::port()   const { return mImpl->port; }
const std::string& HttpTransport::scheme() const { return mImpl->scheme; }
bool               HttpTransport::isHttps() const { return mImpl->isHttps(); }
bool               HttpTransport::configured() const {
    return mImpl->configured;
}

//=========================================================================
// Authentication
//=========================================================================

void HttpTransport::setBearerToken(const std::string& token) {
    mImpl->auth.bearerToken = token;
}

void HttpTransport::clearBearerToken() {
    mImpl->auth.bearerToken.clear();
}

const std::string& HttpTransport::bearerToken() const {
    return mImpl->auth.bearerToken;
}

void HttpTransport::setBasicAuth(const std::string& user,
                                 const std::string& password) {
    mImpl->auth.basicUser     = user;
    mImpl->auth.basicPassword = password;
}

void HttpTransport::clearBasicAuth() {
    mImpl->auth.basicUser.clear();
    mImpl->auth.basicPassword.clear();
}

//=========================================================================
// TLS
//=========================================================================

void HttpTransport::setCaCertFile(const std::string& pemPath) {
    mImpl->tls.caCertFile = pemPath;
    mImpl->tls.caCertData.clear();
}

void HttpTransport::setCaCertData(const std::string& pem) {
    mImpl->tls.caCertData = pem;
    mImpl->tls.caCertFile.clear();
}

void HttpTransport::setVerifyServerCertificate(bool verify) {
    mImpl->tls.verifyServerCert = verify;
}

void HttpTransport::setClientCertificate(const std::string& certPem,
                                         const std::string& keyPem) {
    mImpl->tls.clientCertPem = certPem;
    mImpl->tls.clientKeyPem  = keyPem;
}

const TlsConfig& HttpTransport::tlsConfig() const { return mImpl->tls; }

//=========================================================================
// Logging
//=========================================================================

void HttpTransport::setLogHandler(
    std::function<void(LogLevel, const std::string&)> cb) {
    LockGuard<Mutex> lk(mImpl->logMtx);
    mImpl->logHandler = std::move(cb);
}

void HttpTransport::log(LogLevel lvl, const std::string& msg) const {
    LockGuard<Mutex> lk(mImpl->logMtx);
    if (mImpl->logHandler) mImpl->logHandler(lvl, msg);
}

//=========================================================================
// Synchronous request
//=========================================================================

Result<HttpResponse> HttpTransport::request(const HttpRequest& req,
                                            uint32_t timeoutMs) {
    if (!mImpl->configured) {
        return Error{Error::ConnectionFailed,
                     "base URL not configured -- call setBaseUrl() first"};
    }

    auto cli = mImpl->makeClient(timeoutMs);

    httplib::Headers extraHdrs;
    for (const auto& kv : req.headers) {
        extraHdrs.emplace(kv.first, kv.second);
    }

    httplib::Result res;
    if (req.method == "GET") {
        res = cli->Get(req.path, extraHdrs);
    } else if (req.method == "PUT") {
        res = cli->Put(req.path, extraHdrs, req.body, req.contentType);
    } else if (req.method == "POST") {
        res = cli->Post(req.path, extraHdrs, req.body, req.contentType);
    } else if (req.method == "DELETE") {
        res = cli->Delete(req.path, extraHdrs);
    } else if (req.method == "HEAD") {
        res = cli->Head(req.path, extraHdrs);
    } else {
        return Error{Error::InvalidParameter,
                     "unsupported HTTP method: " + req.method};
    }

    auto out = mImpl->processError(res, req.method, req.path);
    if (out) {
        mImpl->logInfo("%s %s -> %d", req.method.c_str(), req.path.c_str(),
                       out.value().status);
    }
    return out;
}

//=========================================================================
// Streaming request
//=========================================================================

Result<HttpResponse> HttpTransport::streamGet(
    const std::string& path,
    const std::map<std::string, std::string>& extraHeaders,
    uint32_t timeoutMs,
    ContentReceiver receiver) {

    if (!mImpl->configured) {
        return Error{Error::ConnectionFailed,
                     "base URL not configured -- call setBaseUrl() first"};
    }
    if (!receiver) {
        return Error{Error::InvalidParameter,
                     "streamGet requires a content receiver"};
    }

    auto cli = mImpl->makeClient(timeoutMs);

    // Register as the active client so cancelStream() can find it.
    {
        LockGuard<Mutex> lk(mImpl->clientMtx);
        mImpl->activeClient = cli.get();
        mImpl->cancelRequested = false;
    }

    httplib::Headers headers;
    for (const auto& kv : extraHeaders) {
        headers.emplace(kv.first, kv.second);
    }

    // Capture the receiver and the cancel flag.  We do NOT capture the impl
    // pointer because the body callback may run after we have returned.
    std::atomic<bool>* cancelFlag = &mImpl->cancelRequested;
    auto cb = [receiver, cancelFlag](const char* data,
                                     size_t length) -> bool {
        if (cancelFlag->load(std::memory_order_relaxed)) {
            return false;
        }
        return receiver(reinterpret_cast<const uint8_t*>(data), length);
    };

    httplib::Result res = cli->Get(path, headers, cb);

    {
        LockGuard<Mutex> lk(mImpl->clientMtx);
        mImpl->activeClient = nullptr;
    }

    auto out = mImpl->processError(res, "GET", path);
    if (out) {
        // For streaming we deliberately do not return the body; it has
        // already been pushed through the receiver.
        out.value().body.clear();
        mImpl->logInfo("GET %s (stream) -> %d", path.c_str(),
                       out.value().status);
    }
    return out;
}

void HttpTransport::cancelStream() {
    LockGuard<Mutex> lk(mImpl->clientMtx);
    mImpl->cancelRequested = true;
    if (mImpl->activeClient) {
        // cpp-httplib does not have a public Cancel() on Client, but stopping
        // the underlying socket via stop() unblocks any in-flight Get().
        mImpl->activeClient->stop();
    }
}

//=========================================================================
// HTTP status -> itscam::Error mapping
//=========================================================================

Error mapHttpStatusToError(int status, const std::string& message,
                           const std::string& method,
                           const std::string& path) {
    auto base = method + " " + path + ": ";
    std::string msg = message.empty()
                          ? ("HTTP " + std::to_string(status))
                          : message;
    if (status == 401) {
        return Error{Error::NotAuthenticated,
                     base + (message.empty() ? "not authenticated" : message)};
    }
    if (status == 400 || status == 422) {
        return Error{Error::InvalidParameter,
                     base + (message.empty() ? "invalid parameter" : message)};
    }
    if (status == 404) {
        return Error{Error::InvalidParameter,
                     base + (message.empty() ? "not found" : message)};
    }
    if (status == 503) {
        return Error{Error::ConnectionFailed,
                     base + "service unavailable"};
    }
    if (status >= 500) {
        return Error{Error::ServerError, base + msg};
    }
    if (status >= 300) {
        return Error{Error::Unknown, base + "unexpected HTTP " +
                                          std::to_string(status)};
    }
    return Error{Error::Unknown, base + msg};
}

}  // namespace detail
}  // namespace itscam

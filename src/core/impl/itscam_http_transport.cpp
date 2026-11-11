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
#include <cctype>
#include <cstdlib>
#include <chrono>
#include <random>

// A request must not raise SIGPIPE in the host process when the server, or
// a stream stop, has closed the socket: httplib passes these flags to every
// send(), mbedTLS included (a local patch in httplib.h)
#ifndef _WIN32
#include <sys/socket.h>
#ifdef MSG_NOSIGNAL
#define CPPHTTPLIB_SEND_FLAGS MSG_NOSIGNAL
#endif
#endif

// cpp-httplib (header-only).  When the SDK is compiled with TLS support,
// CPPHTTPLIB_MBEDTLS_SUPPORT is defined externally by the build system so
// that httplib pulls in the mbedTLS backend.
#include "../3rdparty/httplib.h"

// Older mingw-w64 i686 import libraries (shipped with the Ubuntu 18.04
// base used by the SDK builder image) do not export _mkgmtime32, which
// cpp-httplib calls through _mkgmtime when time_t is 32-bit.  Provide a
// portable shim so the win-x86 DLL still links.
#if defined(__MINGW32__) && !defined(_WIN64)
extern "C" __time32_t __cdecl _mkgmtime32(struct tm* tm_time) {
    if (!tm_time) return -1;
    static const int kMonthDays[] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    long year  = tm_time->tm_year + 1900;
    long month = tm_time->tm_mon;
    if (month < 0 || month > 11) return -1;
    long day_of_year = kMonthDays[month] + tm_time->tm_mday - 1;
    // Add a leap day if past Feb in a leap year.
    bool leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    if (leap && month > 1) ++day_of_year;
    long years_from_epoch = year - 1970;
    long leap_days = (year - 1) / 4 - (year - 1) / 100 + (year - 1) / 400
                   - (1969 / 4 - 1969 / 100 + 1969 / 400);
    long days = years_from_epoch * 365 + leap_days + day_of_year;
    return static_cast<__time32_t>(
        days * 86400L +
        tm_time->tm_hour * 3600L +
        tm_time->tm_min  * 60L +
        tm_time->tm_sec);
}
#endif

namespace itscam {
namespace detail {

using Mutex = os::Mutex;
template<typename M> using LockGuard = os::LockGuard<M>;

//=========================================================================
// Impl
//=========================================================================

struct HttpTransport::Impl {

    // Guards the target, auth and TLS settings: a stream thread reads them
    // while another thread may log in or reconfigure the client
    mutable Mutex cfgMtx;

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
        std::string host, scheme;
        uint16_t    port;
        AuthConfig  auth;
        TlsConfig   tls;
        {
            LockGuard<Mutex> lk(cfgMtx);
            host   = this->host;
            port   = this->port;
            scheme = this->scheme;
            auth   = this->auth;
            tls    = this->tls;
        }
        const bool https = isHttpsScheme(scheme);

        uint16_t actualPort = port;
        if (actualPort == 0) {
            actualPort = https ? 443 : 80;
        }

        std::unique_ptr<httplib::Client> cli;
        if (https) {
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
            applyTls(cli.get(), tls);
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

    static bool isHttpsScheme(const std::string& scheme) {
        return scheme == "https" || scheme == "HTTPS";
    }

    bool isHttps() const {
        LockGuard<Mutex> lk(cfgMtx);
        return isHttpsScheme(scheme);
    }

    static void applyTls(httplib::Client* cli, const TlsConfig& tls) {
        if (!cli) return;
#if defined(CPPHTTPLIB_OPENSSL_SUPPORT) || defined(CPPHTTPLIB_MBEDTLS_SUPPORT)
        cli->enable_server_certificate_verification(tls.verifyServerCert);
        if (!tls.caCertFile.empty()) {
            cli->set_ca_cert_path(tls.caCertFile);
        }
#else
        (void)cli;
        (void)tls;
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
    {
        LockGuard<Mutex> lk(mImpl->cfgMtx);
        mImpl->host       = host;
        mImpl->port       = port;
        mImpl->scheme     = scheme.empty() ? std::string("http") : scheme;
        mImpl->configured = !host.empty();
    }
    const bool https = mImpl->isHttps();
    mImpl->logInfo("Base URL set to %s://%s:%u",
                   https ? "https" : "http", host.c_str(),
                   static_cast<unsigned>(port != 0 ? port : (https ? 443 : 80)));
}

std::string HttpTransport::host() const {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    return mImpl->host;
}

uint16_t HttpTransport::port() const {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    return mImpl->port;
}

std::string HttpTransport::scheme() const {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    return mImpl->scheme;
}

bool HttpTransport::isHttps() const { return mImpl->isHttps(); }

bool HttpTransport::configured() const {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    return mImpl->configured;
}

//=========================================================================
// Authentication
//=========================================================================

void HttpTransport::setBearerToken(const std::string& token) {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    mImpl->auth.bearerToken = token;
}

void HttpTransport::clearBearerToken() {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    mImpl->auth.bearerToken.clear();
}

std::string HttpTransport::bearerToken() const {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    return mImpl->auth.bearerToken;
}

void HttpTransport::setBasicAuth(const std::string& user,
                                 const std::string& password) {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    mImpl->auth.basicUser     = user;
    mImpl->auth.basicPassword = password;
}

void HttpTransport::clearBasicAuth() {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    mImpl->auth.basicUser.clear();
    mImpl->auth.basicPassword.clear();
}

//=========================================================================
// TLS
//=========================================================================

void HttpTransport::setCaCertFile(const std::string& pemPath) {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    mImpl->tls.caCertFile = pemPath;
    mImpl->tls.caCertData.clear();
}

void HttpTransport::setCaCertData(const std::string& pem) {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    mImpl->tls.caCertData = pem;
    mImpl->tls.caCertFile.clear();
}

void HttpTransport::setVerifyServerCertificate(bool verify) {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    mImpl->tls.verifyServerCert = verify;
}

void HttpTransport::setClientCertificate(const std::string& certPem,
                                         const std::string& keyPem) {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    mImpl->tls.clientCertPem = certPem;
    mImpl->tls.clientKeyPem  = keyPem;
}

TlsConfig HttpTransport::tlsConfig() const {
    LockGuard<Mutex> lk(mImpl->cfgMtx);
    return mImpl->tls;
}

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
    if (!configured()) {
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

struct HttpTransport::StreamCancel::State {
    Mutex             mtx;
    std::atomic<bool> cancelled{false};
    // The stream's client while its streamGet() runs
    httplib::Client*  client = nullptr;
    // The socket while its webSocket() runs
    socket_t          sock = INVALID_SOCKET;
};

HttpTransport::StreamCancel::StreamCancel() : mState(new State) {}

HttpTransport::StreamCancel::~StreamCancel() = default;

void HttpTransport::StreamCancel::cancel() {
    LockGuard<Mutex> lk(mState->mtx);
    mState->cancelled = true;
    if (mState->client) {
        // cpp-httplib does not have a public Cancel() on Client, but stopping
        // the underlying socket via stop() unblocks any in-flight Get().
        mState->client->stop();
    }
    if (mState->sock != INVALID_SOCKET) {
        // Unblocks a webSocket() read; the socket is closed by its owner,
        // only once it is no longer registered here
        httplib::detail::shutdown_socket(mState->sock);
    }
}

Result<HttpResponse> HttpTransport::streamGet(
    const std::string& path,
    const std::map<std::string, std::string>& extraHeaders,
    uint32_t timeoutMs,
    ContentReceiver receiver,
    StreamCancel& cancel) {

    if (!configured()) {
        return Error{Error::ConnectionFailed,
                     "base URL not configured -- call setBaseUrl() first"};
    }
    if (!receiver) {
        return Error{Error::InvalidParameter,
                     "streamGet requires a content receiver"};
    }

    auto cli = mImpl->makeClient(timeoutMs);
    StreamCancel::State& state = *cancel.mState;

    // Under the cancel's lock: a cancel either finds this client or is seen
    // here
    {
        LockGuard<Mutex> lk(state.mtx);
        if (state.cancelled) {
            return Error{Error::Disconnected, "GET " + path + ": cancelled"};
        }
        state.client = cli.get();
    }

    httplib::Headers headers;
    for (const auto& kv : extraHeaders) {
        headers.emplace(kv.first, kv.second);
    }

    // Capture the receiver and the cancel flag.  We do NOT capture the impl
    // pointer because the body callback may run after we have returned.
    std::atomic<bool>* cancelFlag = &state.cancelled;
    auto cb = [receiver, cancelFlag](const char* data,
                                     size_t length) -> bool {
        if (cancelFlag->load(std::memory_order_relaxed)) {
            return false;
        }
        return receiver(reinterpret_cast<const uint8_t*>(data), length);
    };

    httplib::Result res = cli->Get(path, headers, cb);

    {
        LockGuard<Mutex> lk(state.mtx);
        state.client = nullptr;
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

//=========================================================================
// WebSocket (RFC 6455)
//=========================================================================

namespace {

const char kWebSocketGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// Largest message accepted (the telemetry takes a few KB)
const size_t kMaxWebSocketMessage = 16u << 20;

// Largest handshake response accepted
const size_t kMaxWebSocketHandshake = 16u << 10;

// How often a read checks for cancellation and silence
const uint32_t kWebSocketPollMs = 200;

const uint8_t kWsContinuation = 0x0;
const uint8_t kWsText         = 0x1;
const uint8_t kWsBinary       = 0x2;
const uint8_t kWsClose        = 0x8;
const uint8_t kWsPing         = 0x9;
const uint8_t kWsPong         = 0xA;

const uint16_t kWsCloseNormal      = 1000;
const uint16_t kWsCloseProtocol    = 1002;
const uint16_t kWsCloseTooBig      = 1009;

/// SHA-1 (RFC 3174) of @p data as 20 raw bytes, for the handshake only
std::string sha1(const std::string& data) {
    uint32_t h[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u,
                     0xC3D2E1F0u};
    std::string msg = data;
    const uint64_t bits = static_cast<uint64_t>(data.size()) * 8u;
    msg.push_back(static_cast<char>(0x80));
    while (msg.size() % 64 != 56) msg.push_back('\0');
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<char>((bits >> (i * 8)) & 0xFFu));
    }
    auto rol = [](uint32_t v, int n) -> uint32_t {
        return (v << n) | (v >> (32 - n));
    };
    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            const unsigned char* p = reinterpret_cast<const unsigned char*>(
                msg.data() + chunk + static_cast<size_t>(i) * 4);
            w[i] = (static_cast<uint32_t>(p[0]) << 24) |
                   (static_cast<uint32_t>(p[1]) << 16) |
                   (static_cast<uint32_t>(p[2]) << 8) |
                   static_cast<uint32_t>(p[3]);
        }
        for (int i = 16; i < 80; ++i) {
            w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20)      { f = (b & c) | (~b & d);          k = 0x5A827999u; }
            else if (i < 40) { f = b ^ c ^ d;                   k = 0x6ED9EBA1u; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCu; }
            else             { f = b ^ c ^ d;                   k = 0xCA62C1D6u; }
            const uint32_t t = rol(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rol(b, 30);
            b = a;
            a = t;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
    }
    std::string out;
    for (uint32_t v : h) {
        for (int i = 3; i >= 0; --i) {
            out.push_back(static_cast<char>((v >> (i * 8)) & 0xFFu));
        }
    }
    return out;
}

/// Handshake nonces and frame masks: they need to vary, not to be secret
std::string randomBytes(size_t count) {
    static Mutex mtx;
    static std::mt19937 rng{std::random_device{}()};
    LockGuard<Mutex> lk(mtx);
    std::string out(count, '\0');
    for (auto& c : out) c = static_cast<char>(rng() & 0xFFu);
    return out;
}

std::string toLower(std::string s) {
    for (auto& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

std::string trim(const std::string& s) {
    const size_t begin = s.find_first_not_of(" \t");
    if (begin == std::string::npos) return std::string();
    const size_t end = s.find_last_not_of(" \t");
    return s.substr(begin, end - begin + 1);
}

/// An open WebSocket: its socket, TLS session and byte stream
class WebSocketConnection {
public:
    WebSocketConnection() = default;
    WebSocketConnection(const WebSocketConnection&) = delete;
    WebSocketConnection& operator=(const WebSocketConnection&) = delete;
    ~WebSocketConnection() { close(); }

    socket_t sock = INVALID_SOCKET;
#ifdef CPPHTTPLIB_SSL_ENABLED
    httplib::tls::ctx_t     ctx     = nullptr;
    httplib::tls::session_t session = nullptr;
#endif
    std::unique_ptr<httplib::Stream> stream;

    bool writeAll(const char* data, size_t size) {
        while (size > 0) {
            const ssize_t n = stream->write(data, size);
            if (n <= 0) return false;
            data += n;
            size -= static_cast<size_t>(n);
        }
        return true;
    }

    /// Sends one frame, masked as clients must
    bool sendFrame(uint8_t opcode, const std::string& payload) {
        std::string frame;
        frame.push_back(static_cast<char>(0x80u | opcode));
        const uint64_t len = payload.size();
        if (len < 126) {
            frame.push_back(static_cast<char>(0x80u | len));
        } else if (len <= 0xFFFFu) {
            frame.push_back(static_cast<char>(0x80u | 126u));
            frame.push_back(static_cast<char>((len >> 8) & 0xFFu));
            frame.push_back(static_cast<char>(len & 0xFFu));
        } else {
            frame.push_back(static_cast<char>(0x80u | 127u));
            for (int i = 7; i >= 0; --i) {
                frame.push_back(static_cast<char>((len >> (i * 8)) & 0xFFu));
            }
        }
        const std::string mask = randomBytes(4);
        frame += mask;
        for (size_t i = 0; i < payload.size(); ++i) {
            frame.push_back(static_cast<char>(payload[i] ^ mask[i % 4]));
        }
        return writeAll(frame.data(), frame.size());
    }

    bool sendClose(uint16_t code) {
        std::string payload;
        payload.push_back(static_cast<char>((code >> 8) & 0xFFu));
        payload.push_back(static_cast<char>(code & 0xFFu));
        return sendFrame(kWsClose, payload);
    }

    /// Appends what arrives within @p pollMs: > 0 bytes read, 0 none yet,
    /// < 0 the connection closed or failed
    ssize_t readSome(std::string& into, uint32_t pollMs) {
        if (!stream->is_readable()) {
            const ssize_t ready = httplib::detail::select_read(
                sock, static_cast<time_t>(pollMs / 1000),
                static_cast<time_t>((pollMs % 1000) * 1000));
            if (ready == 0) return 0;
            if (ready < 0) return -1;
        }
        // At least the stream's own buffer size, so that it reads straight
        // into ours and is_readable() above stays exact
        char buf[4096];
        const ssize_t n = stream->read(buf, sizeof(buf));
        if (n <= 0) return -1;
        into.append(buf, static_cast<size_t>(n));
        return n;
    }

    void close() {
        stream.reset();
#ifdef CPPHTTPLIB_SSL_ENABLED
        if (session) {
            httplib::tls::shutdown(session, true);
            httplib::tls::free_session(session);
            session = nullptr;
        }
        if (ctx) {
            httplib::tls::free_context(ctx);
            ctx = nullptr;
        }
#endif
        if (sock != INVALID_SOCKET) {
            httplib::detail::shutdown_socket(sock);
            httplib::detail::close_socket(sock);
            sock = INVALID_SOCKET;
        }
    }
};

}  // namespace

Result<void> HttpTransport::webSocket(const std::string& path,
                                      uint32_t timeoutMs,
                                      const std::function<void()>& onOpen,
                                      MessageReceiver receiver,
                                      StreamCancel& cancel) {
    using Clock = std::chrono::steady_clock;

    std::string host, scheme;
    uint16_t    port = 0;
    AuthConfig  auth;
    TlsConfig   tls;
    bool        configured = false;
    {
        LockGuard<Mutex> lk(mImpl->cfgMtx);
        host       = mImpl->host;
        port       = mImpl->port;
        scheme     = mImpl->scheme;
        auth       = mImpl->auth;
        tls        = mImpl->tls;
        configured = mImpl->configured;
    }
    const std::string label = "WebSocket " + path;
    if (!configured) {
        return Error{Error::ConnectionFailed,
                     "base URL not configured -- call setBaseUrl() first"};
    }
    if (!receiver) {
        return Error{Error::InvalidParameter,
                     "webSocket requires a message receiver"};
    }
    if (timeoutMs == 0) timeoutMs = 1;

    const bool https = Impl::isHttpsScheme(scheme);
#ifndef CPPHTTPLIB_SSL_ENABLED
    if (https) {
        return Error{Error::ConnectionFailed,
                     label + ": this SDK build has no TLS support"};
    }
#endif
    const uint16_t defaultPort = https ? 443 : 80;
    const uint16_t actualPort  = port != 0 ? port : defaultPort;

    StreamCancel::State& state = *cancel.mState;
    const Error cancelled{Error::Disconnected, label + ": cancelled"};
    if (state.cancelled) return cancelled;

    const time_t toSec  = static_cast<time_t>(timeoutMs / 1000);
    const time_t toUsec = static_cast<time_t>((timeoutMs % 1000) * 1000);

    // --- Connect -----------------------------------------------------------

    httplib::Error connectError = httplib::Error::Success;
    const socket_t sock = httplib::detail::create_client_socket(
        host, std::string(), actualPort, AF_UNSPEC, true, false,
        httplib::SocketOptions(), toSec, toUsec, toSec, toUsec, toSec, toUsec,
        std::string(), connectError);
    if (sock == INVALID_SOCKET) {
        return Error{connectError == httplib::Error::ConnectionTimeout
                         ? Error::Timeout
                         : Error::ConnectionFailed,
                     label + ": connection failed"};
    }

    WebSocketConnection conn;
    conn.sock = sock;
    {
        LockGuard<Mutex> lk(state.mtx);
        if (state.cancelled) return cancelled;
        state.sock = sock;
    }
    // Destroyed before conn: a cancel never touches the socket once closed
    struct Unregister {
        StreamCancel::State& state;
        ~Unregister() {
            LockGuard<Mutex> lk(state.mtx);
            state.sock = INVALID_SOCKET;
        }
    } unregister{state};

#ifdef CPPHTTPLIB_SSL_ENABLED
    if (https) {
        namespace tlsapi = httplib::tls;
        const Error tlsFailed{Error::ConnectionFailed,
                              label + ": TLS handshake failed"};
        conn.ctx = tlsapi::create_client_context();
        if (!conn.ctx) return tlsFailed;
        if (tls.verifyServerCert) {
            bool loaded = true;
            if (!tls.caCertFile.empty()) {
                loaded = tlsapi::load_ca_file(conn.ctx, tls.caCertFile.c_str());
            } else if (!tls.caCertData.empty()) {
                loaded = tlsapi::load_ca_pem(conn.ctx, tls.caCertData.data(),
                                             tls.caCertData.size());
            } else {
                tlsapi::load_system_certs(conn.ctx);
            }
            if (!loaded) {
                return Error{Error::ConnectionFailed,
                             label + ": failed to load the CA certificates"};
            }
        }
        if (!tls.clientCertPem.empty() && !tls.clientKeyPem.empty() &&
            !tlsapi::set_client_cert_pem(conn.ctx, tls.clientCertPem.c_str(),
                                         tls.clientKeyPem.c_str(), nullptr)) {
            return Error{Error::InvalidParameter,
                         label + ": invalid client certificate"};
        }
        const bool isIp = httplib::detail::is_ip_address(host);
#ifdef CPPHTTPLIB_MBEDTLS_SUPPORT
        // As SSLClient: mbedTLS needs the verification mode set, and checks
        // the name itself only when SNI gives one.  An IP address has no
        // SNI: the chain is still verified, and the checks after the
        // handshake read the result and match the address
        if (tls.verifyServerCert && isIp) {
            tlsapi::set_verify_optional(conn.ctx);
        } else {
            tlsapi::set_verify_client(conn.ctx, tls.verifyServerCert);
        }
#endif
        conn.session = tlsapi::create_session(conn.ctx, sock);
        if (!conn.session) return tlsFailed;
        if (!isIp && !tlsapi::set_sni(conn.session, host.c_str())) {
            return tlsFailed;
        }
        tlsapi::TlsError tlsError;
        if (!tlsapi::connect_nonblocking(conn.session, sock, toSec, toUsec,
                                         &tlsError)) {
            return tlsFailed;
        }
        if (tls.verifyServerCert) {
            if (tlsapi::get_verify_result(conn.session) != 0) {
                return Error{Error::ConnectionFailed,
                             label + ": server certificate rejected"};
            }
            tlsapi::cert_t cert = tlsapi::get_peer_cert(conn.session);
            if (!cert) {
                return Error{Error::ConnectionFailed,
                             label + ": no server certificate"};
            }
            const bool matches = tlsapi::verify_hostname(cert, host.c_str());
            tlsapi::free_cert(cert);
            if (!matches) {
                return Error{Error::ConnectionFailed,
                             label + ": server certificate is not for " + host};
            }
        }
        conn.stream.reset(new httplib::detail::SSLSocketStream(
            sock, conn.session, toSec, toUsec, toSec, toUsec));
    } else
#endif
    {
        conn.stream.reset(new httplib::detail::SocketStream(
            sock, toSec, toUsec, toSec, toUsec));
    }

    // --- Handshake ---------------------------------------------------------

    const std::string key = httplib::detail::base64_encode(randomBytes(16));
    std::string hostHeader =
        host.find(':') != std::string::npos ? "[" + host + "]" : host;
    if (actualPort != defaultPort) {
        hostHeader += ":" + std::to_string(actualPort);
    }
    std::string request = "GET " + path + " HTTP/1.1\r\n"
                          "Host: " + hostHeader + "\r\n"
                          "Upgrade: websocket\r\n"
                          "Connection: Upgrade\r\n"
                          "Sec-WebSocket-Key: " + key + "\r\n"
                          "Sec-WebSocket-Version: 13\r\n"
                          "User-Agent: itscam-sdk\r\n";
    if (!auth.bearerToken.empty()) {
        request += "Authorization: Bearer " + auth.bearerToken + "\r\n";
    } else if (!auth.basicUser.empty()) {
        request += "Authorization: Basic " +
                   httplib::detail::base64_encode(auth.basicUser + ":" +
                                                  auth.basicPassword) +
                   "\r\n";
    }
    request += "\r\n";
    if (!conn.writeAll(request.data(), request.size())) {
        return Error{Error::ConnectionFailed,
                     label + ": failed to send the handshake"};
    }

    std::string in;
    size_t headerEnd = std::string::npos;
    const Clock::time_point handshakeDeadline =
        Clock::now() + std::chrono::milliseconds(timeoutMs);
    while ((headerEnd = in.find("\r\n\r\n")) == std::string::npos) {
        if (state.cancelled) return cancelled;
        if (in.size() > kMaxWebSocketHandshake) {
            return Error{Error::ConnectionFailed,
                         label + ": handshake response too large"};
        }
        if (Clock::now() >= handshakeDeadline) {
            return Error{Error::Timeout, label + ": no handshake response"};
        }
        if (conn.readSome(in, kWebSocketPollMs) < 0) {
            if (state.cancelled) return cancelled;
            return Error{Error::ConnectionFailed,
                         label + ": connection closed during the handshake"};
        }
    }

    int status = 0;
    std::map<std::string, std::string> headers;  // lower-case names
    {
        const std::string head = in.substr(0, headerEnd);
        size_t lineStart = 0;
        bool first = true;
        while (lineStart <= head.size()) {
            size_t lineEnd = head.find("\r\n", lineStart);
            if (lineEnd == std::string::npos) lineEnd = head.size();
            const std::string line = head.substr(lineStart, lineEnd - lineStart);
            if (first) {
                // HTTP/1.1 101 Switching Protocols
                const size_t sp = line.find(' ');
                if (sp != std::string::npos) {
                    status = std::atoi(line.c_str() + sp + 1);
                }
                first = false;
            } else {
                const size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    headers[toLower(trim(line.substr(0, colon)))] =
                        trim(line.substr(colon + 1));
                }
            }
            lineStart = lineEnd + 2;
        }
    }
    if (status != 101) {
        return mapHttpStatusToError(status, std::string(), "GET", path);
    }
    const std::string expectedAccept =
        httplib::detail::base64_encode(sha1(key + kWebSocketGuid));
    if (toLower(headers["upgrade"]) != "websocket" ||
        headers["sec-websocket-accept"] != expectedAccept) {
        return Error{Error::ConnectionFailed,
                     label + ": the server did not accept the WebSocket"};
    }
    in.erase(0, headerEnd + 4);  // frames may follow at once
    if (onOpen) onOpen();

    // --- Frames ------------------------------------------------------------

    const Clock::duration idle = std::chrono::milliseconds(timeoutMs);
    Clock::time_point lastReceived = Clock::now();
    bool pinged = false;
    std::string message;
    bool inMessage = false;

    while (true) {
        // Every complete frame read so far
        while (in.size() >= 2) {
            const uint8_t b0 = static_cast<uint8_t>(in[0]);
            const uint8_t b1 = static_cast<uint8_t>(in[1]);
            const bool    fin    = (b0 & 0x80u) != 0;
            const uint8_t opcode = b0 & 0x0Fu;
            const bool    masked = (b1 & 0x80u) != 0;
            uint64_t len = b1 & 0x7Fu;
            size_t header = 2;
            if (len == 126) {
                if (in.size() < 4) break;
                len = (static_cast<uint64_t>(static_cast<uint8_t>(in[2])) << 8) |
                      static_cast<uint8_t>(in[3]);
                header = 4;
            } else if (len == 127) {
                if (in.size() < 10) break;
                len = 0;
                for (int i = 0; i < 8; ++i) {
                    len = (len << 8) | static_cast<uint8_t>(in[2 + i]);
                }
                header = 10;
            }
            if (len > kMaxWebSocketMessage ||
                (inMessage && message.size() + len > kMaxWebSocketMessage)) {
                conn.sendClose(kWsCloseTooBig);
                return Error{Error::InvalidParameter,
                             label + ": message too large"};
            }
            const size_t maskAt = header;
            if (masked) header += 4;  // servers must not mask; tolerated
            if (in.size() < header + len) break;

            std::string payload = in.substr(header, static_cast<size_t>(len));
            if (masked) {
                for (size_t i = 0; i < payload.size(); ++i) {
                    payload[i] = static_cast<char>(payload[i] ^ in[maskAt + i % 4]);
                }
            }
            in.erase(0, header + static_cast<size_t>(len));
            lastReceived = Clock::now();
            pinged = false;

            const Error protocolError{Error::ServerError,
                                      label + ": WebSocket protocol error"};
            switch (opcode) {
                case kWsText:
                case kWsBinary:
                    if (inMessage) {
                        conn.sendClose(kWsCloseProtocol);
                        return protocolError;
                    }
                    if (fin) {
                        if (!receiver(payload)) {
                            conn.sendClose(kWsCloseNormal);
                            return Result<void>::success();
                        }
                    } else {
                        message = std::move(payload);
                        inMessage = true;
                    }
                    break;
                case kWsContinuation:
                    if (!inMessage) {
                        conn.sendClose(kWsCloseProtocol);
                        return protocolError;
                    }
                    message += payload;
                    if (fin) {
                        inMessage = false;
                        std::string complete;
                        complete.swap(message);
                        if (!receiver(complete)) {
                            conn.sendClose(kWsCloseNormal);
                            return Result<void>::success();
                        }
                    }
                    break;
                case kWsPing:
                    if (!conn.sendFrame(kWsPong, payload)) {
                        return Error{Error::Disconnected,
                                     label + ": connection closed"};
                    }
                    break;
                case kWsPong:
                    break;
                case kWsClose: {
                    // Echo its code, as RFC 6455 5.5.1 asks
                    conn.sendFrame(kWsClose, payload.substr(0, 2));
                    std::string reason = label + ": closed by the server";
                    if (payload.size() >= 2) {
                        const int code = (static_cast<uint8_t>(payload[0]) << 8) |
                                         static_cast<uint8_t>(payload[1]);
                        reason += " (" + std::to_string(code) + ")";
                    }
                    return Error{Error::Disconnected, reason};
                }
                default:
                    conn.sendClose(kWsCloseProtocol);
                    return protocolError;
            }
        }

        if (state.cancelled) {
            conn.sendClose(kWsCloseNormal);
            return Result<void>::success();
        }
        const Clock::duration silence = Clock::now() - lastReceived;
        if (silence >= idle) {
            return Error{Error::Timeout,
                         label + ": nothing received for " +
                             std::to_string(timeoutMs) + " ms"};
        }
        // A silent server is asked to prove it is alive
        if (!pinged && silence >= idle / 2) {
            if (!conn.sendFrame(kWsPing, std::string())) {
                return Error{Error::Disconnected, label + ": connection closed"};
            }
            pinged = true;
        }
        if (conn.readSome(in, kWebSocketPollMs) < 0) {
            if (state.cancelled) return Result<void>::success();
            return Error{Error::Disconnected, label + ": connection closed"};
        }
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

/*
 *  itscam_cgi_client.cpp
 *
 *  ITSCAM Client SDK - CGI client implementation
 *
 *  Copyright (c) 2026 Pumatronix
 */

#include "itscam_cgi_client.h"
#include "impl/itscam_http_transport.h"
#include "impl/itscam_multipart.h"

#include <atomic>
#include <cstring>
#include <sstream>
#include <thread>

namespace itscam {

using Transport = detail::HttpTransport;
using detail::HttpRequest;
using detail::HttpResponse;
using detail::extractMultipartBoundary;
using detail::parseMultipart;
using detail::StreamingMultipartParser;

// ============================================================================
//  Local helpers
// ============================================================================

namespace {

/// Percent-encode an arbitrary byte string for inclusion in a URL query
/// component (RFC 3986 unreserved set only).
std::string urlEncode(const std::string& s) {
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(s.size() * 3);
    for (unsigned char c : s) {
        bool unreserved = (c >= 'A' && c <= 'Z') ||
                          (c >= 'a' && c <= 'z') ||
                          (c >= '0' && c <= '9') ||
                          c == '-' || c == '_' || c == '.' || c == '~';
        if (unreserved) {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0x0F]);
        }
    }
    return out;
}

/// Append "?k=v&..." to @p path based on the supplied query parameters.
/// Empty values are kept (the server treats `?k=` as "set k to empty").
std::string buildQuery(const std::string& path,
                       const std::vector<std::pair<std::string,
                                                   std::string>>& params) {
    std::string out = path;
    bool first = true;
    for (const auto& kv : params) {
        out += first ? '?' : '&';
        first = false;
        out += urlEncode(kv.first);
        out += '=';
        out += urlEncode(kv.second);
    }
    return out;
}

std::string joinCsv(const std::vector<int>& v) {
    std::ostringstream oss;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) oss << ',';
        oss << v[i];
    }
    return oss.str();
}

/// Convert a single-image HttpResponse into a CgiImage.
CgiImage toCgiImage(const HttpResponse& rsp) {
    CgiImage img;
    img.mimeType = rsp.contentType;
    img.data     = rsp.body;
    img.headers  = rsp.headers;
    return img;
}

}  // namespace

// ============================================================================
//  Streaming worker
// ============================================================================

namespace {

class StreamWorker {
public:
    StreamWorker(Transport* transport, std::string path,
                 uint32_t timeoutMs, std::string expectedBoundary,
                 CgiStreamCallback userCallback)
        : mTransport(transport)
        , mPath(std::move(path))
        , mTimeoutMs(timeoutMs)
        , mExpectedBoundary(std::move(expectedBoundary))
        , mUserCallback(std::move(userCallback))
    {}

    ~StreamWorker() {
        stop();
    }

    Result<void> start() {
        if (mRunning.exchange(true)) {
            return Error{Error::InvalidParameter, "stream already running"};
        }
        mShouldStop = false;
        mThread = std::thread(&StreamWorker::run, this);
        return Result<void>::success();
    }

    void stop() {
        if (!mRunning.load()) return;
        mShouldStop = true;
        if (mTransport) mTransport->cancelStream();
        if (mThread.joinable()) mThread.join();
        mRunning = false;
    }

    bool isRunning() const { return mRunning.load(); }

private:
    void run() {
        // First chunk needs to give us the boundary.  We default to the
        // server's well-known value when no Content-Type is available
        // (older firmwares set it on response start, not on the first
        // chunk).
        std::string boundary = mExpectedBoundary;

        std::unique_ptr<StreamingMultipartParser> parser;
        std::atomic<uint64_t> sequence{0};

        auto receiver = [&](const uint8_t* data, size_t length) -> bool {
            if (mShouldStop.load()) return false;
            if (!parser) {
                // Lazily construct once we know the boundary.  If the
                // server's Content-Type was unavailable from the chunk
                // callback (cpp-httplib only exposes it through the
                // Result), we fall back to the expected default.
                parser.reset(new StreamingMultipartParser(
                    boundary,
                    [&](const CgiImage& part) {
                        if (mShouldStop.load()) return;
                        CgiStreamFrame frame;
                        frame.sequence = ++sequence;
                        frame.mimeType = part.mimeType;
                        frame.data     = part.data;
                        frame.headers  = part.headers;
                        if (mUserCallback) mUserCallback(frame);
                    }));
            }
            parser->push(data, length);
            return !mShouldStop.load();
        };

        auto res = mTransport->streamGet(mPath, /*extraHeaders*/{},
                                         mTimeoutMs, receiver);
        // The stream ended (either naturally, on error, or because the
        // caller invoked stop()).  We do not propagate the error -- the
        // caller knows the stream stopped.
        (void)res;
        mRunning = false;
    }

    Transport*          mTransport;
    std::string         mPath;
    uint32_t            mTimeoutMs;
    std::string         mExpectedBoundary;
    CgiStreamCallback   mUserCallback;
    std::thread         mThread;
    std::atomic<bool>   mRunning{false};
    std::atomic<bool>   mShouldStop{false};
};

}  // namespace

// ============================================================================
//  Impl
// ============================================================================

struct ItscamCgiClient::Impl {

    Transport   transport;
    std::string apiPrefix = "/api";

    std::unique_ptr<StreamWorker> mjpegWorker;

    // ----- HTTP helpers ----------------------------------------------------

    Result<HttpResponse> get(const std::string& path, uint32_t timeoutMs) {
        HttpRequest req;
        req.method = "GET";
        req.path   = path;
        return transport.request(req, timeoutMs);
    }

    // ----- Snapshot URL ----------------------------------------------------

    std::string snapshotPath(const SnapshotCgiRequest& r) {
        std::vector<std::pair<std::string, std::string>> params;
        if (!r.shutters.empty()) {
            params.emplace_back("s", joinCsv(r.shutters));
        }
        if (!r.gains.empty()) {
            params.emplace_back("g", joinCsv(r.gains));
        }
        if (r.quality >= 0) {
            params.emplace_back("q", std::to_string(r.quality));
        }
        if (r.mosaic) {
            params.emplace_back("m", "1");
        }
        if (!r.format.empty()) {
            params.emplace_back("fmt", r.format);
        }
        if (r.cenario >= 0) {
            params.emplace_back("c", std::to_string(r.cenario));
        } else if (r.scenario >= 0) {
            params.emplace_back("c", std::to_string(r.scenario));
        }
        if (!r.crop.empty()) {
            params.emplace_back("r", r.crop);
        }
        if (!r.textOverlay.empty()) {
            params.emplace_back("t", r.textOverlay);
        }
        for (const auto& kv : r.userMetadata) {
            std::string key = kv.first;
            if (key.rfind("User_", 0) != 0) key = "User_" + key;
            params.emplace_back(key, kv.second);
        }
        return buildQuery(apiPrefix + "/snapshot.cgi", params);
    }

    // ----- Snapshot response decoder --------------------------------------

    static std::vector<CgiImage> decodeSnapshot(const HttpResponse& rsp) {
        auto boundary = extractMultipartBoundary(rsp.contentType);
        if (!boundary.empty()) {
            auto parts = parseMultipart(rsp.body, boundary);
            if (!parts.empty()) return parts;
        }
        std::vector<CgiImage> out;
        if (!rsp.body.empty()) out.push_back(toCgiImage(rsp));
        return out;
    }
};

// ============================================================================
//  Constructor / destructor / move
// ============================================================================

ItscamCgiClient::ItscamCgiClient() : mImpl(new Impl()) {}

ItscamCgiClient::~ItscamCgiClient() {
    if (mImpl) {
        if (mImpl->mjpegWorker) mImpl->mjpegWorker->stop();
    }
}

ItscamCgiClient::ItscamCgiClient(ItscamCgiClient&&) noexcept = default;
ItscamCgiClient& ItscamCgiClient::operator=(ItscamCgiClient&&) noexcept
    = default;

// ============================================================================
//  Connection / settings
// ============================================================================

void ItscamCgiClient::setBaseUrl(const std::string& host, uint16_t port,
                                 const std::string& scheme) {
    mImpl->transport.setBaseUrl(host, port, scheme);
}

void ItscamCgiClient::setApiPrefix(const std::string& prefix) {
    mImpl->apiPrefix = prefix;
}

// ============================================================================
//  TLS
// ============================================================================

void ItscamCgiClient::setCaCertFile(const std::string& pemPath) {
    mImpl->transport.setCaCertFile(pemPath);
}

void ItscamCgiClient::setCaCertData(const std::string& pem) {
    mImpl->transport.setCaCertData(pem);
}

void ItscamCgiClient::setVerifyServerCertificate(bool verify) {
    mImpl->transport.setVerifyServerCertificate(verify);
}

void ItscamCgiClient::setClientCertificate(const std::string& certPem,
                                           const std::string& keyPem) {
    mImpl->transport.setClientCertificate(certPem, keyPem);
}

// ============================================================================
//  Authentication
// ============================================================================

Result<void> ItscamCgiClient::login(const std::string& username,
                                    const std::string& password,
                                    uint32_t timeoutMs) {
    // The auth endpoint is JSON.  We hand-roll a tiny POST body to avoid
    // pulling in nlohmann::json here.
    std::string body = std::string("{\"params\":{\"username\":\"")
                       + username + "\",\"password\":\"" + password + "\"}}";

    HttpRequest req;
    req.method      = "POST";
    req.path        = mImpl->apiPrefix + "/auth";
    req.body        = std::move(body);
    req.contentType = "application/json";

    auto res = mImpl->transport.request(req, timeoutMs);
    if (!res) return res.error();
    const auto& rsp = res.value();
    if (rsp.status < 200 || rsp.status >= 300) {
        return detail::mapHttpStatusToError(rsp.status, {}, "POST",
                                            req.path);
    }

    // Extract the bearer token from the response body without depending on
    // nlohmann::json here: look for "token":"<value>".
    std::string body_s(rsp.body.begin(), rsp.body.end());
    auto pos = body_s.find("\"token\"");
    if (pos != std::string::npos) {
        pos = body_s.find('"', pos + 7); // after "token"
        if (pos != std::string::npos) {
            auto valStart = body_s.find('"', pos + 1);
            if (valStart != std::string::npos) {
                ++valStart;
                auto valEnd = body_s.find('"', valStart);
                if (valEnd != std::string::npos) {
                    mImpl->transport.setBearerToken(
                        body_s.substr(valStart, valEnd - valStart));
                }
            }
        }
    }
    return Result<void>::success();
}

void ItscamCgiClient::setAuthToken(const std::string& token) {
    mImpl->transport.setBearerToken(token);
}

void ItscamCgiClient::clearAuthToken() {
    mImpl->transport.clearBearerToken();
}

void ItscamCgiClient::setBasicAuth(const std::string& user,
                                   const std::string& password) {
    mImpl->transport.setBasicAuth(user, password);
}

void ItscamCgiClient::clearBasicAuth() {
    mImpl->transport.clearBasicAuth();
}

// ============================================================================
//  /api/lastframe.cgi
// ============================================================================

Result<CgiImage> ItscamCgiClient::getLastFrame(uint32_t timeoutMs) {
    auto res = mImpl->get(mImpl->apiPrefix + "/lastframe.cgi", timeoutMs);
    if (!res) return res.error();
    const auto& rsp = res.value();
    if (rsp.status < 200 || rsp.status >= 300) {
        return detail::mapHttpStatusToError(rsp.status, {}, "GET",
                                            "/api/lastframe.cgi");
    }
    return toCgiImage(rsp);
}

// ============================================================================
//  /api/snapshot.cgi
// ============================================================================

Result<std::vector<CgiImage>> ItscamCgiClient::getSnapshot(
    const SnapshotCgiRequest& request, uint32_t timeoutMs) {
    std::string path = mImpl->snapshotPath(request);
    auto res = mImpl->get(path, timeoutMs);
    if (!res) return res.error();
    const auto& rsp = res.value();
    if (rsp.status < 200 || rsp.status >= 300) {
        return detail::mapHttpStatusToError(rsp.status, {}, "GET", path);
    }
    auto images = Impl::decodeSnapshot(rsp);
    if (images.empty()) {
        return Error{Error::ServerError,
                     "snapshot.cgi returned empty body"};
    }
    return images;
}

Future<std::vector<CgiImage>> ItscamCgiClient::getSnapshotAsync(
    const SnapshotCgiRequest& request, uint32_t timeoutMs) {
    Promise<std::vector<CgiImage>> promise;
    auto future = promise.get_future();
    std::thread([this, request, timeoutMs,
                 promise = std::move(promise)]() mutable {
        promise.set_value(this->getSnapshot(request, timeoutMs));
    }).detach();
    return future;
}

// ============================================================================
//  /api/mjpegvideo.cgi
// ============================================================================

Result<void> ItscamCgiClient::startMjpegStream(CgiStreamCallback callback,
                                               uint32_t timeoutMs) {
    if (mImpl->mjpegWorker && mImpl->mjpegWorker->isRunning()) {
        return Error{Error::InvalidParameter,
                     "mjpeg stream already running"};
    }
    mImpl->mjpegWorker.reset(new StreamWorker(
        &mImpl->transport,
        mImpl->apiPrefix + "/mjpegvideo.cgi",
        timeoutMs,
        "MjpegBoundary",        // server-side default in mjpeg_encoder.h
        std::move(callback)));
    return mImpl->mjpegWorker->start();
}

void ItscamCgiClient::stopMjpegStream() {
    if (mImpl->mjpegWorker) {
        mImpl->mjpegWorker->stop();
        mImpl->mjpegWorker.reset();
    }
}

bool ItscamCgiClient::isMjpegStreamRunning() const {
    return mImpl->mjpegWorker && mImpl->mjpegWorker->isRunning();
}

// ============================================================================
//  /api/trigger.cgi
// ============================================================================

Result<std::string> ItscamCgiClient::forceTrigger(uint32_t timeoutMs) {
    auto path = mImpl->apiPrefix + "/trigger.cgi?force=1";
    auto res  = mImpl->get(path, timeoutMs);
    if (!res) return res.error();
    const auto& rsp = res.value();
    if (rsp.status < 200 || rsp.status >= 300) {
        return detail::mapHttpStatusToError(rsp.status, {}, "GET", path);
    }
    return std::string(rsp.body.begin(), rsp.body.end());
}

// ============================================================================
//  /api/reboot.cgi
// ============================================================================

Result<std::string> ItscamCgiClient::reboot(uint32_t timeoutMs) {
    auto path = mImpl->apiPrefix + "/reboot.cgi";
    auto res  = mImpl->get(path, timeoutMs);
    if (!res) return res.error();
    const auto& rsp = res.value();
    if (rsp.status < 200 || rsp.status >= 300) {
        return detail::mapHttpStatusToError(rsp.status, {}, "GET", path);
    }
    return std::string(rsp.body.begin(), rsp.body.end());
}

// ============================================================================
//  Generic escape hatch
// ============================================================================

Result<CgiResponse> ItscamCgiClient::httpGetRaw(
    const std::string& path,
    const std::map<std::string, std::string>& extraHeaders,
    uint32_t timeoutMs) {
    HttpRequest req;
    req.method  = "GET";
    req.path    = path;
    req.headers = extraHeaders;
    auto res = mImpl->transport.request(req, timeoutMs);
    if (!res) return res.error();
    const auto& rsp = res.value();
    CgiResponse out;
    out.status      = rsp.status;
    out.contentType = rsp.contentType;
    out.body        = rsp.body;
    out.headers     = rsp.headers;
    return out;
}

// ============================================================================
//  Logging
// ============================================================================

void ItscamCgiClient::setLogHandler(
    std::function<void(LogLevel, const std::string&)> cb) {
    mImpl->transport.setLogHandler(std::move(cb));
}

}  // namespace itscam

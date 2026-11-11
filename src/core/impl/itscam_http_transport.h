/*
 *  itscam_http_transport.h
 *
 *  ITSCAM Client SDK - Shared HTTP transport (internal)
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Thin wrapper around cpp-httplib used by ItscamRestClient and
 *  ItscamCgiClient.  Centralises:
 *
 *    - Host / port / scheme storage (http or https)
 *    - Authentication (Bearer JWT or HTTP Basic)
 *    - TLS / mbedTLS configuration
 *    - Per-request client construction with timeouts
 *    - Logging hook
 *    - Common httplib::Result -> itscam::Result<HttpResponse> mapping
 *
 *  This is an INTERNAL header.  It is not installed and not part of the
 *  public SDK surface.
 *
 */
#pragma once

#include "../itscam_types.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace httplib {
class Client;
class ClientImpl;
class SSLClient;
struct Result;
}  // namespace httplib

namespace itscam {
namespace detail {

//=========================================================================
// Plain wire-level HTTP response
//=========================================================================

/// Raw HTTP response surfaced to higher-level clients.  The body is a byte
/// buffer so that binary payloads (JPEG, PNG, multipart bodies) are not
/// corrupted by string conversion.
struct HttpResponse {
    int                                status = 0;       // HTTP status code
    std::string                        contentType;      // Content-Type header
    std::vector<uint8_t>               body;             // raw body bytes
    std::map<std::string, std::string> headers;          // case-insensitive on
                                                         // read
};

//=========================================================================
// HTTP request options
//=========================================================================

struct HttpRequest {
    std::string                        method = "GET";   // GET/PUT/POST/DELETE
    std::string                        path;             // /api/...
    std::string                        body;             // request body
    std::string                        contentType = "application/json";
    std::map<std::string, std::string> headers;          // extra headers
};

//=========================================================================
// Streaming response handler
//=========================================================================

/// Called for each chunk of a streamed response (e.g. multipart MJPEG).
/// Return false to abort the transfer.
using ContentReceiver =
    std::function<bool(const uint8_t* data, size_t length)>;

//=========================================================================
// TLS configuration
//=========================================================================

struct TlsConfig {
    bool        verifyServerCert = true;   // verify the server certificate
    std::string caCertFile;                // path to PEM CA bundle (optional)
    std::string caCertData;                // PEM CA bundle as a string
    std::string clientCertPem;             // client cert (mTLS)
    std::string clientKeyPem;              // client key  (mTLS)
};

//=========================================================================
// Auth configuration
//=========================================================================

struct AuthConfig {
    std::string bearerToken;     // Authorization: Bearer <token>
    std::string basicUser;       // Authorization: Basic <base64(user:pass)>
    std::string basicPassword;
};

//=========================================================================
// HttpTransport
//=========================================================================

class HttpTransport {
public:
    HttpTransport();
    ~HttpTransport();

    // Move-only
    HttpTransport(HttpTransport&&) noexcept;
    HttpTransport& operator=(HttpTransport&&) noexcept;
    HttpTransport(const HttpTransport&) = delete;
    HttpTransport& operator=(const HttpTransport&) = delete;

    // ----- Configuration --------------------------------------------------

    /// Set host / port / scheme.  Scheme must be "http" or "https".  When
    /// port is 0 the protocol default is used (80 for http, 443 for https).
    void setBaseUrl(const std::string& host,
                    uint16_t           port   = 0,
                    const std::string& scheme = "http");

    // Settings may change while other threads use the transport: these
    // return copies
    std::string host()   const;
    uint16_t    port()   const;
    std::string scheme() const;
    bool        isHttps() const;
    bool        configured() const;

    // ----- Authentication -------------------------------------------------

    void setBearerToken(const std::string& token);
    void clearBearerToken();
    std::string bearerToken() const;

    void setBasicAuth(const std::string& user,
                      const std::string& password);
    void clearBasicAuth();

    // ----- TLS ------------------------------------------------------------

    void setCaCertFile(const std::string& pemPath);
    void setCaCertData(const std::string& pem);
    void setVerifyServerCertificate(bool verify);
    void setClientCertificate(const std::string& certPem,
                              const std::string& keyPem);
    TlsConfig tlsConfig() const;

    // ----- Logging --------------------------------------------------------

    void setLogHandler(
        std::function<void(LogLevel, const std::string&)> cb);

    /// Forward an arbitrary log line through the configured handler.
    void log(LogLevel lvl, const std::string& msg) const;

    // ----- Synchronous request --------------------------------------------

    /// Execute a buffered request.  The full response body is collected in
    /// memory and surfaced through HttpResponse.  Maps cpp-httplib errors
    /// onto itscam::Error.
    Result<HttpResponse> request(const HttpRequest& req, uint32_t timeoutMs);

    // ----- Streaming request ----------------------------------------------

    /// Cancels one streamGet() from another thread.  Each stream has its
    /// own, so that a stream still ending never cancels the next one.
    class StreamCancel {
    public:
        StreamCancel();
        ~StreamCancel();
        StreamCancel(const StreamCancel&) = delete;
        StreamCancel& operator=(const StreamCancel&) = delete;

        /// A streamGet() that has not started yet then returns at once.
        /// One in progress returns as soon as its connection notices; this
        /// waits for a connection attempt in progress to end first.
        void cancel();

    private:
        friend class HttpTransport;
        struct State;
        std::unique_ptr<State> mState;
    };

    /// Execute a streaming GET.  Body chunks are pushed to @p receiver as
    /// they arrive.  Returns the final HTTP status / Content-Type via
    /// HttpResponse (with empty body).  @p cancel ends it from another
    /// thread.
    ///
    /// Useful for long-running responses such as snapshot.cgi multipart
    /// bodies, mjpegvideo.cgi multipart/x-mixed-replace, or trigger.cgi
    /// raw streams.
    Result<HttpResponse> streamGet(const std::string& path,
                                   const std::map<std::string, std::string>&
                                       extraHeaders,
                                   uint32_t timeoutMs,
                                   ContentReceiver receiver,
                                   StreamCancel& cancel);

    // ----- WebSocket --------------------------------------------------------

    /// Called with each message of a WebSocket.  Return false to close it.
    using MessageReceiver = std::function<bool(const std::string& message)>;

    /// Open a WebSocket (RFC 6455) on @p path -- ws:// or wss:// as the
    /// base URL's scheme, with the transport's authentication and TLS
    /// settings -- and pass each message to @p receiver, on the calling
    /// thread, until it ends.  @p onOpen runs once the server accepted it.
    ///
    /// When nothing arrives for timeoutMs / 2 the server is pinged (it must
    /// answer), and after timeoutMs of silence the connection is dropped.
    /// Returns success when the receiver or @p cancel closed it, otherwise
    /// the error that ended it: the server closed it (Disconnected), it went
    /// silent (Timeout), it refused the handshake (NotAuthenticated on HTTP
    /// 401, ...) or the connection failed.
    Result<void> webSocket(const std::string& path,
                           uint32_t timeoutMs,
                           const std::function<void()>& onOpen,
                           MessageReceiver receiver,
                           StreamCancel& cancel);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

//=========================================================================
// HTTP status -> itscam::Error mapping
//=========================================================================

/// Map an HTTP status code (plus an optional JSON-decoded "message" field)
/// onto an itscam::Error.  Used by both REST and CGI clients to provide a
/// consistent error surface.
Error mapHttpStatusToError(int status, const std::string& message,
                           const std::string& method,
                           const std::string& path);

}  // namespace detail
}  // namespace itscam

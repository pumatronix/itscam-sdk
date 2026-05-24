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
 *  Requires: cpp-httplib 0.18+ (header-only).
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

// ============================================================================
//  Plain wire-level HTTP response
// ============================================================================

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

// ============================================================================
//  HTTP request options
// ============================================================================

struct HttpRequest {
    std::string                        method = "GET";   // GET/PUT/POST/DELETE
    std::string                        path;             // /api/...
    std::string                        body;             // request body
    std::string                        contentType = "application/json";
    std::map<std::string, std::string> headers;          // extra headers
};

// ============================================================================
//  Streaming response handler
// ============================================================================

/// Called for each chunk of a streamed response (e.g. multipart MJPEG).
/// Return false to abort the transfer.
using ContentReceiver =
    std::function<bool(const uint8_t* data, size_t length)>;

// ============================================================================
//  TLS configuration
// ============================================================================

struct TlsConfig {
    bool        verifyServerCert = true;   // verify the server certificate
    std::string caCertFile;                // path to PEM CA bundle (optional)
    std::string caCertData;                // PEM CA bundle as a string
    std::string clientCertPem;             // client cert (mTLS)
    std::string clientKeyPem;              // client key  (mTLS)
};

// ============================================================================
//  Auth configuration
// ============================================================================

struct AuthConfig {
    std::string bearerToken;     // Authorization: Bearer <token>
    std::string basicUser;       // Authorization: Basic <base64(user:pass)>
    std::string basicPassword;
};

// ============================================================================
//  HttpTransport
// ============================================================================

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

    const std::string& host()   const;
    uint16_t           port()   const;
    const std::string& scheme() const;
    bool               isHttps() const;
    bool               configured() const;

    // ----- Authentication -------------------------------------------------

    void setBearerToken(const std::string& token);
    void clearBearerToken();
    const std::string& bearerToken() const;

    void setBasicAuth(const std::string& user,
                      const std::string& password);
    void clearBasicAuth();

    // ----- TLS ------------------------------------------------------------

    void setCaCertFile(const std::string& pemPath);
    void setCaCertData(const std::string& pem);
    void setVerifyServerCertificate(bool verify);
    void setClientCertificate(const std::string& certPem,
                              const std::string& keyPem);
    const TlsConfig& tlsConfig() const;

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

    /// Execute a streaming GET.  Body chunks are pushed to @p receiver as
    /// they arrive.  Returns the final HTTP status / Content-Type via
    /// HttpResponse (with empty body).
    ///
    /// Useful for long-running responses such as snapshot.cgi multipart
    /// bodies, mjpegvideo.cgi multipart/x-mixed-replace, or trigger.cgi
    /// raw streams.
    Result<HttpResponse> streamGet(const std::string& path,
                                   const std::map<std::string, std::string>&
                                       extraHeaders,
                                   uint32_t timeoutMs,
                                   ContentReceiver receiver);

    /// Cancel any in-flight streamGet on this transport.  Safe to call
    /// from another thread.  After cancelling, streamGet will return as
    /// soon as the underlying connection notices the cancellation.
    void cancelStream();

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

// ============================================================================
//  HTTP status -> itscam::Error mapping
// ============================================================================

/// Map an HTTP status code (plus an optional JSON-decoded "message" field)
/// onto an itscam::Error.  Used by both REST and CGI clients to provide a
/// consistent error surface.
Error mapHttpStatusToError(int status, const std::string& message,
                           const std::string& method,
                           const std::string& path);

}  // namespace detail
}  // namespace itscam

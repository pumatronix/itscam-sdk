/*
 *  itscam_cgi_client.h
 *
 *  ITSCAM Client SDK - CGI client
 *
 *  HTTP / HTTPS client for the legacy ITSCAM CGI endpoints exposed by the
 *  camera daemon and reverse-proxied through the webapp backend:
 *
 *    GET /api/snapshot.cgi      -- on-demand capture (multi-exposure aware)
 *    GET /api/lastframe.cgi     -- most recent preview frame
 *    GET /api/mjpegvideo.cgi    -- continuous MJPEG stream
 *    GET /api/trigger.cgi       -- continuous trigger-image stream
 *    GET /api/reboot.cgi        -- restart the camera-daemon process
 *
 *  Complements ItscamRestClient (JSON config) and ItscamClient (binary TCP).
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Requires: C++14, cpp-httplib 0.18+ (header-only).
 */
#pragma once

#include "itscam_types.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace itscam {

class ItscamCgiClient {
public:
    ItscamCgiClient();
    ~ItscamCgiClient();

    // Move-only (PIMPL)
    ItscamCgiClient(ItscamCgiClient&&) noexcept;
    ItscamCgiClient& operator=(ItscamCgiClient&&) noexcept;
    ItscamCgiClient(const ItscamCgiClient&) = delete;
    ItscamCgiClient& operator=(const ItscamCgiClient&) = delete;

    // =========================================================================
    //  Connection
    // =========================================================================

    /// Configure the target host, port and scheme.  Mirrors
    /// ItscamRestClient::setBaseUrl so the two clients can share a host
    /// configuration.  Pass scheme = "https" to talk to a TLS-enabled
    /// camera (the SDK uses statically-linked mbedTLS).
    void setBaseUrl(const std::string& host,
                    uint16_t port = 80,
                    const std::string& scheme = "http");

    /// Override the path prefix prepended to typed endpoints (default
    /// "/api").  Useful for tests or unusual deployments.
    void setApiPrefix(const std::string& prefix);

    // =========================================================================
    //  TLS (HTTPS) configuration  -- see ItscamRestClient for details
    // =========================================================================

    void setCaCertFile(const std::string& pemPath);
    void setCaCertData(const std::string& pem);
    void setVerifyServerCertificate(bool verify);
    void setClientCertificate(const std::string& certPem,
                              const std::string& keyPem);

    // =========================================================================
    //  Authentication (optional)
    //
    //  The camera daemon's CGI endpoints accept anonymous requests by
    //  default: the webapp's `configCgi.blockAPI` flag is normally set
    //  to `false`, in which case the methods below are not needed.
    //
    //  When the operator opts in to CGI authentication, the endpoints
    //  honour the same credentials as the REST API (JWT bearer, HTTP
    //  Basic).  login() is a convenience that proxies to /api/auth and
    //  stores the returned token internally.
    // =========================================================================

    Result<void> login(const std::string& username,
                       const std::string& password,
                       uint32_t timeoutMs = 10000);

    void setAuthToken(const std::string& token);
    void clearAuthToken();

    void setBasicAuth(const std::string& user, const std::string& password);
    void clearBasicAuth();

    // =========================================================================
    //  /api/lastframe.cgi
    // =========================================================================

    /// Fetch the most recent preview frame from the camera.  Returns a
    /// single CgiImage (JPEG).  Equivalent to the binary client's
    /// getLastFrame() but goes through the CGI server.
    Result<CgiImage> getLastFrame(uint32_t timeoutMs = 10000);

    // =========================================================================
    //  /api/snapshot.cgi
    // =========================================================================

    /// Trigger a snapshot capture and return the resulting image(s).  For
    /// single-exposure requests the vector has one element; for multi-
    /// exposure requests (with @ref SnapshotCgiRequest::mosaic = false) the
    /// vector has one entry per exposure step.  When mosaic = true the
    /// server combines the frames into a single JPEG and the vector has
    /// one element.
    Result<std::vector<CgiImage>> getSnapshot(
        const SnapshotCgiRequest& request,
        uint32_t timeoutMs = 15000);

    /// Async variant.  The returned Future resolves once all exposures
    /// have been received (or the timeout elapses).
    Future<std::vector<CgiImage>> getSnapshotAsync(
        const SnapshotCgiRequest& request,
        uint32_t timeoutMs = 15000);

    // =========================================================================
    //  /api/mjpegvideo.cgi
    // =========================================================================

    /// Begin streaming MJPEG frames in a worker thread.  @p callback is
    /// invoked on the worker thread for each decoded frame; do not block
    /// in the callback.  Use @ref stopMjpegStream() to stop and join the
    /// worker.
    ///
    /// @param callback    Frame consumer.
    /// @param timeoutMs   I/O timeout per HTTP read; controls how quickly
    ///                    network stalls are detected.  Default: 10s.
    Result<void> startMjpegStream(CgiStreamCallback callback,
                                  uint32_t timeoutMs = 10000);

    /// Stop the MJPEG worker thread.  Blocks until the worker has exited.
    void stopMjpegStream();

    /// True iff a stream is currently active.
    bool isMjpegStreamRunning() const;

    // =========================================================================
    //  /api/trigger.cgi
    //
    //  Note: the legacy trigger.cgi stream protocol writes back-to-back
    //  HTTP response messages on a single TCP connection, which cpp-httplib
    //  does not parse correctly.  Continuous trigger frames are exposed
    //  through ItscamClient::onTriggerImage (binary protocol) -- the CGI
    //  surface here only covers the synchronous "force trigger" command.
    // =========================================================================

    /// Issue a one-shot `force=1` request that asks the camera to fire a
    /// trigger right now.  Returns the textual response from the daemon
    /// (typically "forceTriggerRequest").
    Result<std::string> forceTrigger(uint32_t timeoutMs = 10000);

    // =========================================================================
    //  /api/reboot.cgi
    // =========================================================================

    /// Request a camera-daemon reboot.  Returns the textual response from
    /// the daemon ("Rebooting now" on success).
    Result<std::string> reboot(uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Generic escape hatch
    // =========================================================================

    /// Issue an arbitrary GET against any path and surface the raw
    /// response.  Use this for ad-hoc daemon endpoints not covered above.
    Result<CgiResponse> httpGetRaw(
        const std::string& path,
        const std::map<std::string, std::string>& extraHeaders = {},
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Logging
    // =========================================================================

    void setLogHandler(std::function<void(LogLevel,
                                          const std::string&)> cb);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace itscam

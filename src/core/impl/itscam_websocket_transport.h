/*
 *  itscam_websocket_transport.h
 *
 *  ITSCAM Client SDK - minimal internal WebSocket transport
 *
 *  Plain ws:// client used by the software-update status operation.
 *  TLS websocket support is intentionally left for the libwebsockets backend.
 */
#pragma once

#include "../itscam_types.h"

#include <cstdint>
#include <memory>
#include <string>

namespace itscam {
namespace detail {

struct WebSocketOptions {
    std::string host;
    uint16_t    port = 0;
    std::string scheme = "http";
    std::string path;
    std::string bearerToken;
    uint32_t    connectTimeoutMs = 10000;
};

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();

    WebSocketClient(WebSocketClient&&) noexcept;
    WebSocketClient& operator=(WebSocketClient&&) noexcept;
    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;

    Result<void> connect(const WebSocketOptions& options);
    Result<std::string> readText(uint32_t timeoutMs);
    void close();
    void cancel();

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

}  // namespace detail
}  // namespace itscam
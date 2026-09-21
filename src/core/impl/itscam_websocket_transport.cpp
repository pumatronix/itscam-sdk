/*
 *  itscam_websocket_transport.cpp
 *
 *  ITSCAM Client SDK - minimal internal WebSocket transport
 */

#include "itscam_websocket_transport.h"
#include "../itscam_os.h"

#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <random>
#include <sstream>
#include <vector>

namespace itscam {
namespace detail {

namespace {

constexpr const char* kWebSocketGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

std::string trim(const std::string& value) {
    size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }
    size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }
    return value.substr(first, last - first);
}

std::string base64Encode(const unsigned char* data, size_t length) {
    size_t required = 0;
    mbedtls_base64_encode(nullptr, 0, &required, data, length);
    std::string encoded(required, '\0');
    size_t written = 0;
    if (mbedtls_base64_encode(reinterpret_cast<unsigned char*>(&encoded[0]),
                              encoded.size(), &written, data, length) != 0) {
        return std::string();
    }
    encoded.resize(written);
    return encoded;
}

std::string makeWebSocketKey() {
    std::array<unsigned char, 16> bytes{};
    std::random_device rd;
    for (auto& byte : bytes) {
        byte = static_cast<unsigned char>(rd());
    }
    return base64Encode(bytes.data(), bytes.size());
}

std::string makeAcceptKey(const std::string& key) {
    const std::string input = key + kWebSocketGuid;
    unsigned char digest[20]{};
    if (mbedtls_sha1(reinterpret_cast<const unsigned char*>(input.data()),
                    input.size(), digest) != 0) {
        return std::string();
    }
    return base64Encode(digest, sizeof(digest));
}

uint64_t decodeUint(const std::vector<uint8_t>& bytes, size_t offset,
                    size_t count) {
    uint64_t value = 0;
    for (size_t i = 0; i < count; ++i) {
        value = (value << 8) | bytes[offset + i];
    }
    return value;
}

}  // namespace

struct WebSocketClient::Impl {
    os::SocketHandle sock = ITSCAM_OS_INVALID_SOCKET;
    os::Mutex        sockMtx;
    std::vector<uint8_t> pending;

    bool isOpen() const {
        return sock != ITSCAM_OS_INVALID_SOCKET;
    }

    void closeSocket() {
        os::LockGuard<os::Mutex> lk(sockMtx);
        if (sock != ITSCAM_OS_INVALID_SOCKET) {
            os::socketClose(sock);
            sock = ITSCAM_OS_INVALID_SOCKET;
        }
    }

    Result<void> waitWritable(uint32_t timeoutMs) {
        int ready = os::socketWait(sock, static_cast<int>(timeoutMs), true);
        if (ready == 0) return Error{Error::Timeout, "websocket connect timed out"};
        if (ready < 0) return Error{Error::ConnectionFailed, "websocket connect failed"};
        return Result<void>::success();
    }

    Result<void> sendAll(const uint8_t* data, size_t length, uint32_t timeoutMs) {
        size_t sent = 0;
        while (sent < length) {
            int ready = os::socketWait(sock, static_cast<int>(timeoutMs), true);
            if (ready == 0) return Error{Error::Timeout, "websocket send timed out"};
            if (ready < 0) return Error{Error::ConnectionFailed, "websocket send failed"};

            int rc = os::socketSend(sock, data + sent, length - sent);
            if (rc > 0) {
                sent += static_cast<size_t>(rc);
                continue;
            }
            return Error{Error::ConnectionFailed, "websocket send failed"};
        }
        return Result<void>::success();
    }

    Result<void> sendAll(const std::string& text, uint32_t timeoutMs) {
        return sendAll(reinterpret_cast<const uint8_t*>(text.data()),
                       text.size(), timeoutMs);
    }

    Result<void> readMore(uint32_t timeoutMs) {
        int ready = os::socketWait(sock, static_cast<int>(timeoutMs), false);
        if (ready == 0) return Error{Error::Timeout, "websocket read timed out"};
        if (ready < 0) return Error{Error::ConnectionFailed, "websocket read failed"};

        uint8_t buffer[2048];
        int n = os::socketRead(sock, buffer, sizeof(buffer));
        if (n <= 0) return Error{Error::Disconnected, "websocket disconnected"};
        pending.insert(pending.end(), buffer, buffer + n);
        return Result<void>::success();
    }

    Result<void> readUntilHeaders(uint32_t timeoutMs, std::string& headers) {
        const uint64_t deadline = os::monotonicMs() + timeoutMs;
        while (pending.size() < 16384) {
            auto it = std::search(pending.begin(), pending.end(),
                                  reinterpret_cast<const uint8_t*>("\r\n\r\n"),
                                  reinterpret_cast<const uint8_t*>("\r\n\r\n") + 4);
            if (it != pending.end()) {
                const size_t headerEnd = static_cast<size_t>(it - pending.begin()) + 4;
                headers.assign(pending.begin(), pending.begin() + headerEnd);
                pending.erase(pending.begin(), pending.begin() + headerEnd);
                return Result<void>::success();
            }

            uint64_t now = os::monotonicMs();
            if (now >= deadline) {
                return Error{Error::Timeout, "websocket handshake timed out"};
            }
            uint32_t waitMs = static_cast<uint32_t>(deadline - now);
            auto read = readMore(waitMs);
            if (!read) return read.error();
        }
        return Error{Error::ServerError, "websocket handshake response is too large"};
    }

    Result<void> validateHandshake(const std::string& response,
                                   const std::string& expectedAccept) {
        std::istringstream stream(response);
        std::string line;
        if (!std::getline(stream, line)) {
            return Error{Error::ConnectionFailed, "websocket handshake failed"};
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.find(" 101 ") == std::string::npos) {
            return Error{Error::ConnectionFailed,
                         "websocket upgrade was rejected: " + line};
        }

        std::string accept;
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            auto sep = line.find(':');
            if (sep == std::string::npos) continue;
            std::string name = toLower(trim(line.substr(0, sep)));
            std::string value = trim(line.substr(sep + 1));
            if (name == "sec-websocket-accept") {
                accept = value;
            }
        }

        if (accept != expectedAccept) {
            return Error{Error::ConnectionFailed,
                         "websocket handshake accept key mismatch"};
        }
        return Result<void>::success();
    }

    Result<void> ensureBytes(size_t count, uint32_t timeoutMs) {
        while (pending.size() < count) {
            auto read = readMore(timeoutMs);
            if (!read) return read.error();
        }
        return Result<void>::success();
    }

    Result<void> sendFrame(uint8_t opcode, const std::vector<uint8_t>& payload) {
        std::vector<uint8_t> frame;
        frame.push_back(static_cast<uint8_t>(0x80 | opcode));

        if (payload.size() < 126) {
            frame.push_back(static_cast<uint8_t>(0x80 | payload.size()));
        } else if (payload.size() <= 0xffff) {
            frame.push_back(0x80 | 126);
            frame.push_back(static_cast<uint8_t>((payload.size() >> 8) & 0xff));
            frame.push_back(static_cast<uint8_t>(payload.size() & 0xff));
        } else {
            frame.push_back(0x80 | 127);
            uint64_t length = payload.size();
            for (int shift = 56; shift >= 0; shift -= 8) {
                frame.push_back(static_cast<uint8_t>((length >> shift) & 0xff));
            }
        }

        std::array<uint8_t, 4> mask{};
        std::random_device rd;
        for (auto& byte : mask) byte = static_cast<uint8_t>(rd());
        frame.insert(frame.end(), mask.begin(), mask.end());
        for (size_t i = 0; i < payload.size(); ++i) {
            frame.push_back(static_cast<uint8_t>(payload[i] ^ mask[i % mask.size()]));
        }
        return sendAll(frame.data(), frame.size(), 10000);
    }

    Result<std::string> readText(uint32_t timeoutMs) {
        std::vector<uint8_t> assembled;
        bool assembling = false;

        while (true) {
            auto header = ensureBytes(2, timeoutMs);
            if (!header) return header.error();

            uint8_t b0 = pending[0];
            uint8_t b1 = pending[1];
            bool fin = (b0 & 0x80) != 0;
            uint8_t opcode = b0 & 0x0f;
            bool masked = (b1 & 0x80) != 0;
            uint64_t payloadLength = b1 & 0x7f;
            size_t offset = 2;

            if (payloadLength == 126) {
                auto ext = ensureBytes(offset + 2, timeoutMs);
                if (!ext) return ext.error();
                payloadLength = decodeUint(pending, offset, 2);
                offset += 2;
            } else if (payloadLength == 127) {
                auto ext = ensureBytes(offset + 8, timeoutMs);
                if (!ext) return ext.error();
                payloadLength = decodeUint(pending, offset, 8);
                offset += 8;
            }

            std::array<uint8_t, 4> mask{};
            if (masked) {
                auto maskBytes = ensureBytes(offset + mask.size(), timeoutMs);
                if (!maskBytes) return maskBytes.error();
                std::copy(pending.begin() + offset,
                          pending.begin() + offset + mask.size(),
                          mask.begin());
                offset += mask.size();
            }

            if (payloadLength > 16 * 1024 * 1024) {
                return Error{Error::ServerError, "websocket frame is too large"};
            }

            auto body = ensureBytes(offset + static_cast<size_t>(payloadLength), timeoutMs);
            if (!body) return body.error();

            std::vector<uint8_t> payload(
                pending.begin() + offset,
                pending.begin() + offset + static_cast<size_t>(payloadLength));
            pending.erase(pending.begin(),
                          pending.begin() + offset + static_cast<size_t>(payloadLength));

            if (masked) {
                for (size_t i = 0; i < payload.size(); ++i) {
                    payload[i] ^= mask[i % mask.size()];
                }
            }

            if (opcode == 0x8) {
                sendFrame(0x8, {});
                closeSocket();
                return Error{Error::Disconnected, "websocket closed"};
            }
            if (opcode == 0x9) {
                sendFrame(0xA, payload);
                continue;
            }
            if (opcode == 0xA) {
                continue;
            }
            if (opcode == 0x1) {
                if (fin) {
                    return std::string(payload.begin(), payload.end());
                }
                assembled = std::move(payload);
                assembling = true;
                continue;
            }
            if (opcode == 0x0 && assembling) {
                assembled.insert(assembled.end(), payload.begin(), payload.end());
                if (fin) {
                    return std::string(assembled.begin(), assembled.end());
                }
                continue;
            }
        }
    }
};

WebSocketClient::WebSocketClient() : mImpl(new Impl()) {}
WebSocketClient::~WebSocketClient() = default;
WebSocketClient::WebSocketClient(WebSocketClient&&) noexcept = default;
WebSocketClient& WebSocketClient::operator=(WebSocketClient&&) noexcept = default;

Result<void> WebSocketClient::connect(const WebSocketOptions& options) {
    if (options.scheme == "https" || options.scheme == "wss") {
        return Error{Error::ConnectionFailed,
                     "software-update websocket over TLS requires libwebsockets backend"};
    }
    if (options.host.empty() || options.path.empty()) {
        return Error{Error::InvalidParameter, "websocket host and path are required"};
    }

    uint16_t port = options.port ? options.port : 80;
    mImpl->sock = os::socketCreate();
    if (mImpl->sock == ITSCAM_OS_INVALID_SOCKET) {
        return Error{Error::ConnectionFailed, "failed to create websocket socket"};
    }
    os::socketSetNonblocking(mImpl->sock, true);

    int rc = os::socketConnect(mImpl->sock, options.host.c_str(), port);
    if (rc == ITSCAM_SOCK_EINPROGRESS) {
        auto writable = mImpl->waitWritable(options.connectTimeoutMs);
        if (!writable) {
            mImpl->closeSocket();
            return writable.error();
        }
    } else if (rc != 0) {
        mImpl->closeSocket();
        return Error{Error::ConnectionFailed,
                     "websocket connect failed; plain status websocket currently requires an IPv4 address"};
    }

    const std::string key = makeWebSocketKey();
    const std::string expectedAccept = makeAcceptKey(key);
    std::ostringstream req;
    req << "GET " << options.path << " HTTP/1.1\r\n"
        << "Host: " << options.host << ":" << port << "\r\n"
        << "Upgrade: websocket\r\n"
        << "Connection: Upgrade\r\n"
        << "Sec-WebSocket-Key: " << key << "\r\n"
        << "Sec-WebSocket-Version: 13\r\n";
    if (!options.bearerToken.empty()) {
        req << "Authorization: Bearer " << options.bearerToken << "\r\n";
    }
    req << "\r\n";

    auto sent = mImpl->sendAll(req.str(), options.connectTimeoutMs);
    if (!sent) {
        mImpl->closeSocket();
        return sent.error();
    }

    std::string response;
    auto read = mImpl->readUntilHeaders(options.connectTimeoutMs, response);
    if (!read) {
        mImpl->closeSocket();
        return read.error();
    }

    auto valid = mImpl->validateHandshake(response, expectedAccept);
    if (!valid) {
        mImpl->closeSocket();
        return valid.error();
    }

    return Result<void>::success();
}

Result<std::string> WebSocketClient::readText(uint32_t timeoutMs) {
    if (!mImpl->isOpen()) {
        return Error{Error::Disconnected, "websocket is not connected"};
    }
    return mImpl->readText(timeoutMs);
}

void WebSocketClient::close() {
    if (!mImpl->isOpen()) return;
    mImpl->sendFrame(0x8, {});
    mImpl->closeSocket();
}

void WebSocketClient::cancel() {
    mImpl->closeSocket();
}

}  // namespace detail
}  // namespace itscam
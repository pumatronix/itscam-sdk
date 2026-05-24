/*
 *  itscam_client.cpp
 *
 *  ITSCAM Client SDK - Full implementation
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Wire-protocol compatible with the Cougar server daemon.
 *  Supports: Linux (POSIX sockets), Windows (Winsock2).
 */

//=========================================================================
// Includes
//=========================================================================

#include "itscam_client.h"
#include "itscam_jpeg_utils.h"
#include "itscam_os.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <map>
#include <stdarg.h>
#include <vector>

//=========================================================================
// Platform-specific socket includes (for socket API constants and types)
//=========================================================================

#if defined(_WIN32) || defined(__MINGW32__)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0600
    #endif
#endif

// Use OS abstraction types
using socket_t = itscam_os::SocketHandle;

// Use OS abstraction types for threading
using Mutex = itscam_os::Mutex;
using ConditionVariable = itscam_os::ConditionVariable;
using Thread = itscam_os::Thread;
template<typename M> using LockGuard = itscam_os::LockGuard<M>;
template<typename M> using UniqueLock = itscam_os::UniqueLock<M>;

//=========================================================================
// Lightweight Promise/Future for internal wire message exchange
// (Named WirePromise/WireFuture to avoid collision with itscam::Future)
//=========================================================================

template<typename T>
class WirePromise;

template<typename T>
class WireFuture {
public:
    WireFuture() : m_state(nullptr) {}

    WireFuture(WireFuture&& other) noexcept : m_state(other.m_state) {
        other.m_state = nullptr;
    }

    WireFuture& operator=(WireFuture&& other) noexcept {
        if (this != &other) {
            m_state = other.m_state;
            other.m_state = nullptr;
        }
        return *this;
    }

    WireFuture(const WireFuture&) = delete;
    WireFuture& operator=(const WireFuture&) = delete;

    // Wait for result with timeout
    enum class Status { ready, timeout };
    Status waitFor(uint32_t timeout_ms) {
        if (!m_state) return Status::timeout;
        UniqueLock<Mutex> lk(m_state->mtx);
        if (m_state->ready) return Status::ready;
        auto result = m_state->cv.waitFor(lk, timeout_ms);
        return (result == ConditionVariable::CvStatus::timeout) 
            ? Status::timeout : Status::ready;
    }

    // Get the result (must be ready)
    T get() {
        if (!m_state) return T{};
        LockGuard<Mutex> lk(m_state->mtx);
        return std::move(m_state->value);
    }

    bool valid() const { return m_state != nullptr; }

private:
    friend class WirePromise<T>;

    struct SharedState {
        Mutex mtx;
        ConditionVariable cv;
        T value;
        bool ready = false;
    };

    std::shared_ptr<SharedState> m_state;

    explicit WireFuture(std::shared_ptr<SharedState> state) : m_state(std::move(state)) {}
};

template<typename T>
class WirePromise {
public:
    WirePromise() : m_state(std::make_shared<typename WireFuture<T>::SharedState>()) {}

    WirePromise(WirePromise&& other) noexcept : m_state(std::move(other.m_state)) {}

    WirePromise& operator=(WirePromise&& other) noexcept {
        if (this != &other) {
            m_state = std::move(other.m_state);
        }
        return *this;
    }

    WirePromise(const WirePromise&) = delete;
    WirePromise& operator=(const WirePromise&) = delete;

    WireFuture<T> get_future() {
        return WireFuture<T>(m_state);
    }

    void set_value(T value) {
        if (!m_state) return;
        {
            LockGuard<Mutex> lk(m_state->mtx);
            m_state->value = std::move(value);
            m_state->ready = true;
        }
        m_state->cv.notifyAll();
    }

private:
    std::shared_ptr<typename WireFuture<T>::SharedState> m_state;
};

namespace itscam {

//=========================================================================
// CRC-16/XMODEM table  (identical to the server implementation)
//=========================================================================

static const uint16_t kCrc16Tab[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

static uint16_t calcXModem(const uint8_t* data, uint32_t len,
                           uint16_t init = 0x0000) {
    uint16_t crc = init;
    for (uint32_t i = 0; i < len; ++i) {
        uint8_t tmp = ((crc >> 8) ^ data[i]) & 0xFF;
        crc = (crc << 8) ^ kCrc16Tab[tmp];
    }
    return crc;
}

//=========================================================================
// Wire protocol constants  (shared with Cougar server)
//=========================================================================

static constexpr size_t   HEADER_SIZE     = 13;
static constexpr size_t   MAX_BODY_SIZE   = 0xFFFFFF;
static constexpr size_t   CRC_SIZE        = 2;
static constexpr uint32_t ID_INCREMENT    = 2;
static constexpr uint32_t ID_CLIENT_OFFSET = 0;
static constexpr uint8_t  START_BYTE      = 0x66;
static constexpr size_t   TCP_BUF_SIZE    = 0x10000;

//=========================================================================
// Protocol opcodes
//=========================================================================

enum Op : uint16_t {
    OP_NACK             = 1,
    OP_SHUTDOWN         = 256,
    OP_EVT_TRIGGER      = 257,
    OP_JPEG_TRIGGER     = 258,
    OP_EVT_SNAPSHOT     = 259,
    OP_JPEG_SNAPSHOT    = 260,
    OP_EVT_PREVIEW      = 261,
    OP_JPEG_PREVIEW     = 262,
    OP_EVT_PIPE_START   = 263,
    OP_EVT_GPIO         = 264,
    OP_EVT_SERIAL       = 265,
    OP_IMGPKG_TRIGGER   = 266,
    OP_IMGPKG_SNAPSHOT  = 267,
    OP_SET_OPT_STR      = 512,
    OP_SET_CALLBACKS    = 513,
    OP_SET_JPEG_CFGS    = 514,
    OP_TRIGGER_SNAPSHOT = 515,
    OP_GET_LASTFRAME    = 516,
    OP_AUTHENTICATE     = 517,
    OP_SET_SERIAL_CFGS  = 518,
    OP_SEND_SERIAL_DATA = 519,
    OP_SET_EQUIP_CFGS   = 520,
    OP_CMD_REBOOT       = 521,
};

//=========================================================================
// Internal wire message
//=========================================================================

struct WireMsg {
    uint16_t op = 0;
    uint32_t id = 0;
    std::vector<uint8_t> body;
};

//=========================================================================
// Serialisation / deserialisation
//=========================================================================

static std::vector<uint8_t> serializeMsg(const WireMsg& msg) {
    std::vector<uint8_t> ret(msg.body.size() + HEADER_SIZE +
                              (msg.body.empty() ? 0 : CRC_SIZE));
    ret[0] = START_BYTE;
    *(uint32_t*)(&ret[1]) = itscam_os::hostToNet32((uint32_t)msg.body.size());
    *(uint16_t*)(&ret[5]) = itscam_os::hostToNet16(msg.op);
    *(uint32_t*)(&ret[7]) = itscam_os::hostToNet32(msg.id);
    *(uint16_t*)(&ret[11]) = itscam_os::hostToNet16(calcXModem(&ret[0], HEADER_SIZE - CRC_SIZE));
    if (!msg.body.empty()) {
        std::memcpy(&ret[HEADER_SIZE], msg.body.data(), msg.body.size());
        *(uint16_t*)(&ret[HEADER_SIZE + msg.body.size()]) =
            itscam_os::hostToNet16(calcXModem(msg.body.data(), (uint32_t)msg.body.size()));
    }
    return ret;
}

static void compactRawBuffer(std::vector<uint8_t>& raw, size_t& offset) {
    if (offset == 0) {
        return;
    }

    const size_t remaining = raw.size() - offset;
    if (remaining > 0) {
        std::memmove(raw.data(), raw.data() + offset, remaining);
    }
    raw.resize(remaining);
    offset = 0;
}

static bool readOneMsg(std::vector<uint8_t>& raw, size_t& offset, WireMsg& msg) {
    size_t off = offset;
    while (true) {
        for (; off < raw.size() && raw[off] != START_BYTE; ++off);
        if (raw.size() < off + HEADER_SIZE) {
            offset = off;
            return false;
        }
        if (calcXModem(&raw[off], HEADER_SIZE) != 0x0000) {
            ++off;
            continue;
        }
        uint8_t* hdr = &raw[off];
        uint32_t bodySize = itscam_os::netToHost32(*(uint32_t*)(hdr + 1));
        if (bodySize > MAX_BODY_SIZE) { ++off; continue; }
        msg.op = itscam_os::netToHost16(*(uint16_t*)(hdr + 5));
        msg.id = itscam_os::netToHost32(*(uint32_t*)(hdr + 7));
        auto bodyBegin = raw.begin() + off + HEADER_SIZE;
        if (bodySize > 0) {
            if (off + HEADER_SIZE + bodySize + CRC_SIZE > raw.size()) {
                offset = off;
                return false;
            }
            if (calcXModem(hdr + HEADER_SIZE, bodySize + CRC_SIZE) != 0x0000) {
                ++off;
                continue;
            }
            msg.body.assign(bodyBegin, bodyBegin + bodySize);
            offset = off + HEADER_SIZE + bodySize + CRC_SIZE;
        } else {
            msg.body.clear();
            offset = off + HEADER_SIZE;
        }
        return true;
    }
}

// JSON serialisation (JM_PLAIN only -- hardcoded as per design)
static std::vector<uint8_t> jsonToBody(const nlohmann::json& j) {
    std::string s = j.dump();
    return std::vector<uint8_t>(s.begin(), s.end());
}

static nlohmann::json bodyToJson(const std::vector<uint8_t>& body) {
    if (body.empty()) return nlohmann::json::object();
    std::string s(body.begin(), body.end());
    auto j = nlohmann::json::parse(s, nullptr, false);
    if (j.is_discarded()) return nlohmann::json::object();
    return j;
}

//=========================================================================
// Thread-safe message queue
//=========================================================================

struct MsgQueue {
    std::deque<WireMsg> queue;
    Mutex mtx;
    ConditionVariable cv;

    void push(WireMsg& m) {
        {
            LockGuard<Mutex> lk(mtx);
            queue.push_back(std::move(m));
        }
        cv.notifyOne();
    }

    bool pop(WireMsg& m, uint32_t timeoutMs) {
        UniqueLock<Mutex> lk(mtx);
        while (queue.empty()) {
            if (cv.waitFor(lk, timeoutMs) == ConditionVariable::CvStatus::timeout)
                return false;
        }
        m = std::move(queue.front());
        queue.pop_front();
        return true;
    }

    int size() {
        LockGuard<Mutex> lk(mtx);
        return (int)queue.size();
    }
};

//=========================================================================
// SerialPort  <-->  protocol string helpers
//=========================================================================

static const char* serialPortToStr(SerialPort p) {
    return p == SerialPort::Serial2 ? "serial2" : "serial1";
}

static SerialPort strToSerialPort(const std::string& s) {
    return (s == "serial2") ? SerialPort::Serial2 : SerialPort::Serial1;
}

//=========================================================================
// JSON  <-->  domain struct helpers
//=========================================================================

/// Read a JSON field as boolean, tolerating integer 0/1 values
/// that older firmware may send instead of true/false.
static bool jsonBoolField(const nlohmann::json& j, const char* key, bool dflt = false) {
    if (!j.count(key)) return dflt;
    auto& v = j[key];
    if (v.is_boolean()) return v.get<bool>();
    if (v.is_number())  return v.get<int>() != 0;
    return dflt;
}

/// Read a JSON field as string, tolerating numeric values
/// that older firmware may send instead of strings.
static std::string jsonStrField(const nlohmann::json& j, const char* key,
                                 const std::string& dflt = "") {
    if (!j.count(key)) return dflt;
    auto& v = j[key];
    if (v.is_string())  return v.get<std::string>();
    if (v.is_number_integer()) return std::to_string(v.get<int64_t>());
    if (v.is_number_float())   return std::to_string(v.get<double>());
    if (v.is_boolean()) return v.get<bool>() ? "true" : "false";
    if (v.is_null())    return dflt;
    return dflt;
}

static FrameInfo parseFrameInfo(const nlohmann::json& j) {
    FrameInfo fi;
    if (j.count("rid"))        fi.requestId      = j["rid"].get<uint64_t>();
    if (j.count("framecount")) fi.frameCount     = j["framecount"].get<uint64_t>();
    if (j.count("ogSize")) {
        fi.originalSize.width  = j["ogSize"].value("w", 0);
        fi.originalSize.height = j["ogSize"].value("h", 0);
    }
    if (j.count("size")) {
        fi.size.width  = j["size"].value("w", 0);
        fi.size.height = j["size"].value("h", 0);
    }
    if (j.count("crop")) {
        fi.cropOffset.x = j["crop"].value("x", 0);
        fi.cropOffset.y = j["crop"].value("y", 0);
    }
    fi.shutter = j.value("shutter", 0);
    fi.gain    = j.value("gain", 0.0f);
    if (j.count("multExp")) {
        fi.multiExpLength = j["multExp"].value("len", 0);
        fi.multiExpIndex  = j["multExp"].value("pos", 0);
    }
    if (j.count("date")) {
        auto& d = j["date"];
        fi.timestamp.year  = d.value("year",  0);
        fi.timestamp.month = d.value("month", 0);
        fi.timestamp.day   = d.value("day",   0);
        fi.timestamp.hour  = d.value("hour",  0);
        fi.timestamp.min   = d.value("min",   0);
        fi.timestamp.sec   = d.value("sec",   0);
        fi.timestamp.msec  = d.value("msec",  0);
    }
    if (j.count("time")) {
        auto& t = j["time"];
        fi.timing.now           = t.value("now",   (uint64_t)0);
        fi.timing.dmaTransfer   = t.value("dma",   (uint64_t)0);
        fi.timing.exposureSetup = t.value("setup", (uint64_t)0);
        fi.timing.exposureEnd   = t.value("exp",   (uint64_t)0);
    }
    if (j.count("stats")) {
        auto& s = j["stats"];
        fi.stats.level  = s.value("level",  0.0f);
        fi.stats.meanR  = s.value("meanr",  0.0f);
        fi.stats.meanG  = s.value("meang",  0.0f);
        fi.stats.meanB  = s.value("meanb",  0.0f);
        fi.stats.stdDev = s.value("stddev", 0.0f);
    }
    if (j.count("io")) {
        auto& io = j["io"];
        fi.gpio.input[0]  = jsonBoolField(io, "gpIn1");
        fi.gpio.input[1]  = jsonBoolField(io, "gpIn2");
        fi.gpio.input[2]  = jsonBoolField(io, "gpIn3");
        fi.gpio.input[3]  = jsonBoolField(io, "gpIn4");
        fi.gpio.output[0] = jsonBoolField(io, "gpOut1");
        fi.gpio.output[1] = jsonBoolField(io, "gpOut2");
        fi.gpio.output[2] = jsonBoolField(io, "gpOut3");
        fi.gpio.output[3] = jsonBoolField(io, "gpOut4");
    }
    if (j.count("jidosha") && j["jidosha"].is_array()) {
        for (auto& r : j["jidosha"]) {
            PlateRecognition pr;
            pr.plate = jsonStrField(r, "plate");
            if (r.count("probs") && r["probs"].is_array()) {
                for (auto& p : r["probs"]) pr.charProbabilities.push_back(p.get<float>());
            }
            if (r.count("pos")) {
                pr.x = r["pos"].value("x", 0); pr.y = r["pos"].value("y", 0);
                pr.width = r["pos"].value("w", 0); pr.height = r["pos"].value("h", 0);
            }
            pr.color = jsonStrField(r, "color");
            pr.isMotorcycle = r.value("moto", 0) != 0;
            pr.countryCode = r.value("country", 0);
            fi.plates.push_back(std::move(pr));
        }
    }
    if (j.count("classifier") && j["classifier"].is_array()) {
        for (auto& c : j["classifier"]) {
            ObjectDetection od;
            od.type = c.value("type", 0);
            od.probability = c.value("prob", 0.0f);
            if (c.count("pos")) {
                od.x = c["pos"].value("x", 0); od.y = c["pos"].value("y", 0);
                od.width = c["pos"].value("w", 0); od.height = c["pos"].value("h", 0);
            }
            od.brand = jsonStrField(c, "brand"); od.model = jsonStrField(c, "model");
            od.color = jsonStrField(c, "color");
            od.brandProb = c.value("brandProb", 0.0f);
            od.modelProb = c.value("modelProb", 0.0f);
            od.colorProb = c.value("colorProb", 0.0f);
            od.trackerSpeed = c.value("trackerSpeed", 0);
            od.lane = jsonStrField(c, "lane");
            fi.objects.push_back(std::move(od));
        }
    }
    return fi;
}

/// Parse a "mixed" message: 4-byte big-endian metadata length, then metadata
/// JSON, then the rest is the JPEG payload.
static CaptureResult parseMixedBody(const std::vector<uint8_t>& body) {
    CaptureResult cr;
    if (body.size() <= 4) return cr;
    uint32_t metaLen = itscam_os::netToHost32(*(const uint32_t*)body.data());
    if (body.size() < 4 + metaLen) return cr;
    std::vector<uint8_t> metaRaw(body.begin() + 4, body.begin() + 4 + metaLen);
    nlohmann::json meta = bodyToJson(metaRaw);
    cr.info.frameCount = meta.value("framecount", (uint64_t)0);
    cr.quality = meta.value("quality", 0);
    if (meta.count("rid")) cr.info.requestId = meta["rid"].get<uint64_t>();
    if (meta.count("ogSize")) {
        cr.info.originalSize.width  = meta["ogSize"].value("w", 0);
        cr.info.originalSize.height = meta["ogSize"].value("h", 0);
    }
    if (meta.count("size")) {
        cr.info.size.width  = meta["size"].value("w", 0);
        cr.info.size.height = meta["size"].value("h", 0);
    }
    if (meta.count("crop")) {
        cr.info.cropOffset.x = meta["crop"].value("x", 0);
        cr.info.cropOffset.y = meta["crop"].value("y", 0);
    }
    cr.info.shutter = meta.value("shutter", 0);
    cr.info.gain    = meta.value("gain", 0.0f);
    if (meta.count("multExp")) {
        cr.info.multiExpLength = meta["multExp"].value("len", 0);
        cr.info.multiExpIndex  = meta["multExp"].value("pos", 0);
    }
    if (meta.count("date")) {
        auto& d = meta["date"];
        cr.info.timestamp.year  = d.value("year",  0);
        cr.info.timestamp.month = d.value("month", 0);
        cr.info.timestamp.day   = d.value("day",   0);
        cr.info.timestamp.hour  = d.value("hour",  0);
        cr.info.timestamp.min   = d.value("min",   0);
        cr.info.timestamp.sec   = d.value("sec",   0);
        cr.info.timestamp.msec  = d.value("msec",  0);
    }
    if (meta.count("time")) {
        auto& t = meta["time"];
        cr.info.timing.now           = t.value("now",   (uint64_t)0);
        cr.info.timing.dmaTransfer   = t.value("dma",   (uint64_t)0);
        cr.info.timing.exposureSetup = t.value("setup", (uint64_t)0);
        cr.info.timing.exposureEnd   = t.value("exp",   (uint64_t)0);
    }
    if (meta.count("stats")) {
        auto& s = meta["stats"];
        cr.info.stats.level  = s.value("level",  0.0f);
        cr.info.stats.meanR  = s.value("meanr",  0.0f);
        cr.info.stats.meanG  = s.value("meang",  0.0f);
        cr.info.stats.meanB  = s.value("meanb",  0.0f);
        cr.info.stats.stdDev = s.value("stddev", 0.0f);
    }
    if (meta.count("io")) {
        auto& io = meta["io"];
        cr.info.gpio.input[0]  = jsonBoolField(io, "gpIn1");
        cr.info.gpio.input[1]  = jsonBoolField(io, "gpIn2");
        cr.info.gpio.input[2]  = jsonBoolField(io, "gpIn3");
        cr.info.gpio.input[3]  = jsonBoolField(io, "gpIn4");
        cr.info.gpio.output[0] = jsonBoolField(io, "gpOut1");
        cr.info.gpio.output[1] = jsonBoolField(io, "gpOut2");
        cr.info.gpio.output[2] = jsonBoolField(io, "gpOut3");
        cr.info.gpio.output[3] = jsonBoolField(io, "gpOut4");
    }
    if (meta.count("tags") && meta["tags"].is_object()) {
        for (auto it = meta["tags"].begin(); it != meta["tags"].end(); ++it) {
            // Tags may be strings or numbers depending on firmware version
            if (it.value().is_string()) {
                cr.tags[it.key()] = it.value().get<std::string>();
            } else if (it.value().is_number_integer()) {
                cr.tags[it.key()] = std::to_string(it.value().get<int64_t>());
            } else if (it.value().is_number_float()) {
                cr.tags[it.key()] = std::to_string(it.value().get<double>());
            }
        }
    }
    // Parse plate recognition and classifier metadata (included since protocol
    // update — these arrays are also present in the EVT_TRIGGER JSON event).
    if (meta.count("jidosha") && meta["jidosha"].is_array()) {
        for (auto& r : meta["jidosha"]) {
            PlateRecognition pr;
            pr.plate = jsonStrField(r, "plate");
            if (r.count("probs") && r["probs"].is_array()) {
                for (auto& p : r["probs"]) pr.charProbabilities.push_back(p.get<float>());
            }
            if (r.count("pos")) {
                pr.x = r["pos"].value("x", 0); pr.y = r["pos"].value("y", 0);
                pr.width = r["pos"].value("w", 0); pr.height = r["pos"].value("h", 0);
            }
            pr.color = jsonStrField(r, "color");
            pr.isMotorcycle = r.value("moto", 0) != 0;
            pr.countryCode = r.value("country", 0);
            cr.info.plates.push_back(std::move(pr));
        }
    }
    if (meta.count("classifier") && meta["classifier"].is_array()) {
        for (auto& c : meta["classifier"]) {
            ObjectDetection od;
            od.type = c.value("type", 0);
            od.probability = c.value("prob", 0.0f);
            if (c.count("pos")) {
                od.x = c["pos"].value("x", 0); od.y = c["pos"].value("y", 0);
                od.width = c["pos"].value("w", 0); od.height = c["pos"].value("h", 0);
            }
            od.brand = jsonStrField(c, "brand"); od.model = jsonStrField(c, "model");
            od.color = jsonStrField(c, "color");
            od.brandProb = c.value("brandProb", 0.0f);
            od.modelProb = c.value("modelProb", 0.0f);
            od.colorProb = c.value("colorProb", 0.0f);
            od.trackerSpeed = c.value("trackerSpeed", 0);
            od.lane = jsonStrField(c, "lane");
            cr.info.objects.push_back(std::move(od));
        }
    }
    cr.jpeg.assign(body.begin() + 4 + metaLen, body.end());
    return cr;
}

//=========================================================================
// ItscamClient::Impl
//=========================================================================

struct ItscamClient::Impl {
    // ---- State ---
    // DISCONNECTING = user explicitly called disconnect(); never auto-reconnect.
    enum State { IDLE, CONNECTING, RUNNING, STOPPING, DISCONNECTING, STOPPED };
    std::atomic<int> state{IDLE};

    std::string address;
    uint16_t port = 60000;
    socket_t sock = ITSCAM_OS_INVALID_SOCKET;

    // ---- Threading ----
    Thread socketThread;
    Thread handlerThread;
    MsgQueue inboundQueue;

    // ---- Outbound serialisation ----
    Mutex sendMtx;
    Mutex idMtx;
    uint32_t idCounter = ID_CLIENT_OFFSET;

    // ---- Promise map for request-response ----
    Mutex promiseMtx;
    std::map<uint32_t, WirePromise<WireMsg>> promises;

    // ---- Pending captureSnapshot exposure groups (RID -> group) ----
    struct PendingCaptureExposureGroup {
        std::vector<CaptureResult> frames;
        int expectedCount = 0;     // from multiExpLength; 0 = unknown yet
        bool complete = false;
        ConditionVariable cv;
    };
    Mutex captureMtx;
    std::map<uint64_t, std::shared_ptr<PendingCaptureExposureGroup>> pendingCaptures;

    // ---- Exposure group accumulator for event callbacks ----
    struct ExposureGroupAccumulator {
        uint64_t rid = 0;
        int expectedCount = 0;
        std::vector<CaptureResult> frames;
        uint64_t firstFrameTimeMs = 0;  // monotonic time in milliseconds
        void reset() {
            rid = 0; expectedCount = 0; frames.clear(); firstFrameTimeMs = 0;
        }
    };
    ExposureGroupAccumulator triggerExpGroupAcc;
    ExposureGroupAccumulator snapshotExpGroupAcc;

    // ---- EVT_TRIGGER metadata cache for merging with IMGPKG events ----
    // When the remote camera runs older firmware, the IMGPKG mixed body
    // may lack jidosha/classifier metadata.  The EVT_TRIGGER JSON event
    // always includes this data, so we cache it by RID and merge into
    // the CaptureResult when needed.
    static const size_t TRIGGER_INFO_CACHE_MAX = 32;
    std::map<uint64_t, FrameInfo> triggerInfoCache;  // guarded by cbMtx

    void cacheTriggerInfo(const FrameInfo& fi) {
        // Only cache if there's actually detection metadata to merge
        if (fi.plates.empty() && fi.objects.empty()) return;
        if (fi.requestId == 0) return;

        triggerInfoCache[fi.requestId] = fi;

        // Evict oldest entries if cache grows too large
        while (triggerInfoCache.size() > TRIGGER_INFO_CACHE_MAX) {
            triggerInfoCache.erase(triggerInfoCache.begin());
        }
    }

    /// Merge cached EVT_TRIGGER metadata into a CaptureResult if it lacks
    /// plate/classifier data.  Also applies JPEG comment fallback as last resort.
    ///
    /// @return A diagnostic string describing which source was used (or why
    ///         all failed).  Empty if metadata was already present.
    ///         NOTE: caller must hold cbMtx.
    std::string mergeDetectionMetadata(CaptureResult& cr) {
        if (!cr.info.plates.empty() || !cr.info.objects.empty()) return {};

        // Try 1: merge from cached EVT_TRIGGER event (keyed by RID).
        // Do NOT erase after merge — in multi-exposure groups all exposures
        // share the same RID and each needs the metadata.  The cache
        // self-evicts by size in cacheTriggerInfo().
        auto it = triggerInfoCache.find(cr.info.requestId);
        if (it != triggerInfoCache.end()) {
            cr.info.plates  = it->second.plates;
            cr.info.objects = it->second.objects;
            return "EVT_TRIGGER cache";
        }

        // Try 2: parse from JPEG COM markers (oldest firmware fallback)
        if (populateFromJpegComment(cr)) {
            return "JPEG COM";
        }

        // All fallbacks exhausted — build diagnostic string
        if (!cr.jpeg.empty()) {
            size_t sz = cr.jpeg.size();
            uint8_t lastByte  = cr.jpeg[sz - 1];
            uint8_t last2Byte = sz >= 2 ? cr.jpeg[sz - 2] : 0;
            std::string comRaw = extractJpegComment(cr.jpeg);
            char diag[256];
            snprintf(diag, sizeof(diag),
                "no metadata: jpeg=%zuB tail=0x%02X%02X com=%s",
                sz, (unsigned)last2Byte, (unsigned)lastByte,
                comRaw.empty() ? "(none)" : "(present but no plates)");
            return diag;
        }
        return "no metadata: jpeg empty";
    }

    // ---- Callbacks ----
    Mutex cbMtx;
    std::function<void(const FrameInfo&)>       cbTriggerMetadata;
    std::function<void(const FrameInfo&)>       cbSnapshotMetadata;
    std::function<void(const FrameInfo&)>       cbPreviewMetadata;
    std::function<void(const CaptureResult&)>   cbTriggerImage;
    std::function<void(const CaptureResult&)>   cbSnapshotImage;
    std::function<void(const CaptureResult&)>   cbPreviewImage;
    std::function<void(const std::vector<CaptureResult>&)> cbTriggerExposureGroup;
    std::function<void(const std::vector<CaptureResult>&)> cbSnapshotExposureGroup;
    std::function<void(const FrameInfo&)>       cbPipelineStart;
    std::function<void(const GpioEvent&)>       cbGpio;
    std::function<void(const SerialData&)>      cbSerial;
    std::function<void(const std::string&)>     cbDisconnect;
    std::function<void(ConnectionState, const std::string&)> cbConnectionState;
    std::function<void(LogLevel, const std::string&)> cbLog;

    // ---- Config ----
    std::atomic<uint32_t> pingInterval{5};
    std::atomic<uint32_t> defaultTimeout{10000};
    std::atomic<uint32_t> exposureGroupTimeoutMs{5000};
    uint64_t lastSentTime{0};  // monotonic time in milliseconds
    std::atomic<uint32_t> lastPingId{0};
    std::atomic<uint32_t> maxPingFailures{10};   // 0 = disabled
    uint32_t consecutivePingFailures = 0;        // guarded by read-loop single-thread

    // ---- Imgpkg tracking for unified image callbacks ----
    bool imgpkgActive = false; // updated whenever setJpegConfig is called

    // ---- Auto-reconnect ----
    AutoReconnectConfig reconnectCfg;
    std::string lastPassword;
    EventSubscription lastSubscription{};
    JpegConfig lastJpegConfig{};
    TriggerConfig lastTriggerConfig{};
    uint32_t lastTriggerProfileId = 0;
    bool hasPassword       = false;
    bool isAuthenticated   = false;  // true after authenticate() succeeds; reset on disconnect
    bool hasSubscription   = false;
    bool hasJpegConfig     = false;
    bool hasTriggerConfig  = false;
    bool hadSuccessfulSession = false;  // true once a session has been established

    // Returns true when a command should be deferred because we know the server
    // requires authentication that hasn't been performed yet this session.
    bool pendingAuth() const { return hasPassword && !isAuthenticated; }

    bool canQueueForReplay() const {
        int st = state.load();
        return reconnectCfg.enabled &&
               st != IDLE &&
               st != STOPPED &&
               st != DISCONNECTING;
    }

    // -----------------------------------------------------------------------
    // Logging
    // -----------------------------------------------------------------------
    void log(LogLevel lvl, const char* fmt, ...) {
        LockGuard<Mutex> lk(cbMtx);
        if (!cbLog) return;
        char buf[512];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        cbLog(lvl, buf);
    }

    // -----------------------------------------------------------------------
    // Socket send helpers
    // -----------------------------------------------------------------------
    bool sendRaw(const std::vector<uint8_t>& data) {
        LockGuard<Mutex> lk(sendMtx);
        if (sock == ITSCAM_OS_INVALID_SOCKET) return false;
        int64_t n = itscam_os::socketSend(sock, data.data(), data.size());
        lastSentTime = itscam_os::monotonicNow();
        if (n == -1) state.store(STOPPING);
        return n == (int64_t)data.size();
    }

    uint32_t nextId() {
        LockGuard<Mutex> lk(idMtx);
        uint32_t id = idCounter;
        idCounter += ID_INCREMENT;
        return id;
    }

    // -----------------------------------------------------------------------
    // Send a request and register a promise for the response
    // -----------------------------------------------------------------------
    bool sendRequest(uint16_t op, const nlohmann::json& params, uint32_t& outId) {
        outId = nextId();
        WireMsg m;
        m.op = op;
        m.id = outId;
        m.body = jsonToBody(params);
        {
            LockGuard<Mutex> lk(promiseMtx);
            promises[outId] = WirePromise<WireMsg>();
        }
        if (!sendRaw(serializeMsg(m))) {
            LockGuard<Mutex> lk(promiseMtx);
            promises.erase(outId);
            return false;
        }
        return true;
    }

    // -----------------------------------------------------------------------
    // Wait for a response to a previously-sent request
    // -----------------------------------------------------------------------
    Result<nlohmann::json> waitResponse(uint16_t expectedOp, uint32_t id,
                                        uint32_t timeoutMs) {
        WireFuture<WireMsg> fut;
        {
            LockGuard<Mutex> lk(promiseMtx);
            auto it = promises.find(id);
            if (it == promises.end())
                return Error{Error::Unknown, "promise not found"};
            fut = it->second.get_future();
        }
        if (fut.waitFor(timeoutMs) == WireFuture<WireMsg>::Status::timeout) {
            LockGuard<Mutex> lk(promiseMtx);
            promises.erase(id);
            return Error{Error::Timeout, "request timed out"};
        }
        WireMsg resp = fut.get();
        {
            LockGuard<Mutex> lk(promiseMtx);
            promises.erase(id);
        }
        if (resp.op == OP_NACK && expectedOp != OP_NACK) {
            nlohmann::json nack = bodyToJson(resp.body);
            return Error{Error::ServerError,
                         nack.value("reason", "unknown server error")};
        }
        if (resp.op != expectedOp && resp.op != OP_NACK) {
            return Error{Error::ServerError, "unexpected response opcode"};
        }
        return bodyToJson(resp.body);
    }

    // -----------------------------------------------------------------------
    // Synchronous request helper  (send + wait)
    // -----------------------------------------------------------------------
    Result<nlohmann::json> syncCall(uint16_t op, const nlohmann::json& params,
                                    uint32_t timeoutMs) {
        uint32_t id;
        if (!sendRequest(op, params, id))
            return Error{Error::ConnectionFailed, "failed to send"};
        return waitResponse(op, id, timeoutMs);
    }

    // -----------------------------------------------------------------------
    // syncCallDirect()  --  send a request and read the response directly
    // from the socket, bypassing the inbound queue.
    //
    // Only call this before runReadLoop() has started (e.g. during session
    // replay), when no other thread is reading from the socket.  Any
    // interleaved messages that are not the awaited response are pushed into
    // inboundQueue so the handler thread can process them once it starts.
    // -----------------------------------------------------------------------
    Result<nlohmann::json> syncCallDirect(uint16_t op, const nlohmann::json& params,
                                          uint32_t timeoutMs) {
        uint32_t id = nextId();
        WireMsg req;
        req.op  = op;
        req.id  = id;
        req.body = jsonToBody(params);
        if (!sendRaw(serializeMsg(req)))
            return Error{Error::ConnectionFailed, "failed to send"};

        std::vector<uint8_t> rawBuf;
        size_t rawOffset = 0;
        uint8_t readBuf[TCP_BUF_SIZE];
        uint64_t deadline = itscam_os::monotonicNow() + timeoutMs;

        while (itscam_os::monotonicNow() < deadline) {
            uint64_t remaining = deadline - itscam_os::monotonicNow();
            int waitMs = static_cast<int>(std::min(remaining, (uint64_t)1000));
            int waitResult = itscam_os::socketWait(sock, waitMs, false);
            if (waitResult < 0) return Error{Error::ConnectionFailed, "socket error"};
            if (waitResult == 0) continue;

            int64_t n = itscam_os::socketRead(sock, readBuf, TCP_BUF_SIZE);
            if (n == 0) return Error{Error::ConnectionFailed, "socket disconnected"};
            if (n  < 0) return Error{Error::ConnectionFailed, "socket read error"};

            rawBuf.insert(rawBuf.end(), readBuf, readBuf + n);
            WireMsg resp;
            while (readOneMsg(rawBuf, rawOffset, resp)) {
                if (resp.id == id) {
                    if (resp.op == OP_NACK) {
                        nlohmann::json nack = bodyToJson(resp.body);
                        return Error{Error::ServerError,
                                     nack.value("reason", "unknown server error")};
                    }
                    return bodyToJson(resp.body);
                }
                // Buffer interleaved messages for the handler thread
                inboundQueue.push(resp);
            }
        }
        return Error{Error::Timeout, "request timed out"};
    }

    // -----------------------------------------------------------------------
    // Asynchronous request helper
    // -----------------------------------------------------------------------
    itscam::Future<nlohmann::json> asyncCall(uint16_t op, const nlohmann::json& params,
                                     uint32_t timeoutMs) {
        auto outer = std::make_shared<itscam::Promise<nlohmann::json>>();
        auto outerFut = outer->get_future();

        uint32_t id;
        if (!sendRequest(op, params, id)) {
            outer->set_value(
                Result<nlohmann::json>(Error{Error::ConnectionFailed, "failed to send"}));
            return outerFut;
        }

        // Spawn a detached waiter thread that will resolve the outer promise
        auto* self = this;
        uint16_t expectedOp = op;
        Thread([self, expectedOp, id, timeoutMs, outer]() {
            try {
                outer->set_value(self->waitResponse(expectedOp, id, timeoutMs));
            } catch (const std::exception& ex) {
                try {
                    outer->set_value(Error{Error::Unknown, std::string("exception: ") + ex.what()});
                } catch (...) {}
            } catch (...) {
                try {
                    outer->set_value(Error{Error::Unknown, "unknown exception"});
                } catch (...) {}
            }
        }).detach();

        return outerFut;
    }

    // -----------------------------------------------------------------------
    // Socket thread  --  TCP connect + read loop
    // -----------------------------------------------------------------------
    void runSocketThread() {
        log(LogLevel::Info, "Connecting to %s:%d ...", address.c_str(), port);

        bool initialConnected = tryConnect();

        if (!initialConnected) {
            // Initial connection failed — if auto-reconnect is NOT enabled, give up
            if (!reconnectCfg.enabled) {
                fireConnectionState(ConnectionState::Disconnected, "initial connection failed");
                state.store(STOPPED);
                return;
            }
            // Auto-reconnect enabled: fall through to the reconnect loop below
            log(LogLevel::Info, "Initial connection failed, will retry (auto-reconnect enabled)");
            fireConnectionState(ConnectionState::Reconnecting, "initial connection failed, retrying");
        }

        if (initialConnected) {
            // Replay any stored configuration via direct socket I/O before going
            // live. The socket thread has exclusive read access at this point so
            // syncCallDirect() works without any helper thread.
            if (!replaySession()) {
                log(LogLevel::Error, "Initial session setup failed");
                closeSocket();
                fireConnectionState(ConnectionState::Disconnected, "initial session setup failed");
                state.store(STOPPED);
                return;
            }

            state.store(RUNNING);
            lastSentTime = itscam_os::monotonicNow();
            hadSuccessfulSession = true;
            log(LogLevel::Info, "Connected");
            fireConnectionState(ConnectionState::Connected, "connected");

            // Start message handler
            handlerThread = Thread([this]{
                try { runHandlerThread(); }
                catch (const std::exception& ex) {
                    log(LogLevel::Error, "Handler thread exception: %s", ex.what());
                } catch (...) {
                    log(LogLevel::Error, "Handler thread unknown exception");
                }
            });

            // Read loop (returns when state != RUNNING)
            runReadLoop();

            // ---- Connection lost or user disconnect ----
            int exitState = state.load();
            cleanupAfterDisconnect();

            // If user explicitly called disconnect(), do not retry
            if (exitState == DISCONNECTING || !reconnectCfg.enabled) {
                state.store(STOPPED);
                return;
            }
        }

        // ---- Auto-reconnect loop ----
        uint32_t maxRetries = reconnectCfg.maxRetries;
        uint32_t attempt = 0;
        int exitState = IDLE;

        while (maxRetries == 0 || attempt < maxRetries) {
            // Check if user called disconnect() while we were waiting
            if (state.load() == DISCONNECTING) break;

            ++attempt;
            log(LogLevel::Info, "Reconnecting (attempt %u)...", attempt);
            fireConnectionState(ConnectionState::Reconnecting,
                                "attempt " + std::to_string(attempt));

            // Interruptible sleep: check state every 100ms during the interval
            {
                uint64_t sleepEnd = itscam_os::monotonicNow() + reconnectCfg.intervalMs;
                while (itscam_os::monotonicNow() < sleepEnd) {
                    if (state.load() == DISCONNECTING) break;
                    itscam_os::sleepForMs(100);
                }
                if (state.load() == DISCONNECTING) break;
            }

            if (!tryConnect()) continue;

            // Replay session configuration before going live.
            // Direct socket I/O — the socket thread has exclusive read access
            // here, so no helper thread is needed.
            if (!replaySession()) {
                log(LogLevel::Error, "Session replay failed, will retry");
                closeSocket();
                continue;
            }

            state.store(RUNNING);
            lastSentTime = itscam_os::monotonicNow();
            log(LogLevel::Info, "Reconnected (TCP)");

            // Start a new handler thread for this connection
            handlerThread = Thread([this]{
                try { runHandlerThread(); }
                catch (const std::exception& ex) {
                    log(LogLevel::Error, "Handler thread exception: %s", ex.what());
                } catch (...) {
                    log(LogLevel::Error, "Handler thread unknown exception");
                }
            });

            if (hadSuccessfulSession) {
                log(LogLevel::Info, "Session restored");
                fireConnectionState(ConnectionState::Reconnected, "session restored");
            } else {
                // First successful connection after initial failure
                hadSuccessfulSession = true;
                log(LogLevel::Info, "Connected (after retry)");
                fireConnectionState(ConnectionState::Connected, "connected");
            }

            // Socket thread owns the read loop
            runReadLoop();

            exitState = state.load();
            cleanupAfterDisconnect();

            if (exitState == DISCONNECTING) break;

            // Reset attempt counter after a successful connection that later dropped
            attempt = 0;
        }

        state.store(STOPPED);
    }

    void closeSocket() {
        LockGuard<Mutex> lk(sendMtx);
        if (sock != ITSCAM_OS_INVALID_SOCKET) {
            itscam_os::socketClose(sock);
            sock = ITSCAM_OS_INVALID_SOCKET;
        }
    }

    // -----------------------------------------------------------------------
    // Connection state callback helper
    // -----------------------------------------------------------------------
    void fireConnectionState(ConnectionState cs, const std::string& reason) {
        LockGuard<Mutex> lk(cbMtx);
        safeCallback("connectionState", cbConnectionState, cs, reason);
    }

    // -----------------------------------------------------------------------
    // tryConnect()  --  TCP socket creation + non-blocking connect
    // Returns true if TCP is up; sets sock and leaves it in blocking mode.
    // -----------------------------------------------------------------------
    bool tryConnect() {
        {
            LockGuard<Mutex> lk(sendMtx);
            sock = itscam_os::socketCreate();
            if (sock == ITSCAM_OS_INVALID_SOCKET) {
                int err = itscam_os::socketErrno();
                log(LogLevel::Error, "Socket creation failed (%d: %s)",
                    err, itscam_os::socketStrerror(err));
                return false;
            }
        }

        // Set non-blocking mode for connect with timeout
        itscam_os::socketSetNonblocking(sock, true);
        int ret = itscam_os::socketConnect(sock, address.c_str(), port);
        int connectErr = itscam_os::socketErrno();
        if (ret < 0 && connectErr != ITSCAM_SOCK_EINPROGRESS && connectErr != ITSCAM_SOCK_EWOULDBLOCK) {
            log(LogLevel::Error, "Connect failed (%d: %s)", connectErr, itscam_os::socketStrerror(connectErr));
            closeSocket();
            return false;
        }

        // Wait for connection to complete
        // socketWait returns: 1 = socket ready, 0 = timeout, -1 = error
        int waitResult = itscam_os::socketWait(sock, 3000, true);  // 3 second timeout, check write
        if (waitResult <= 0) {
            log(LogLevel::Error, "Connect timed out or error");
            closeSocket();
            return false;
        }

        // Check for socket error
        int sockErr = itscam_os::socketGetError(sock);
        if (sockErr != ITSCAM_SOCK_OK) {
            log(LogLevel::Error, "Connect error: %s", itscam_os::socketStrerror(sockErr));
            closeSocket();
            return false;
        }

        // Return to blocking mode for reads
        itscam_os::socketSetNonblocking(sock, false);
        return true;
    }

    bool applyStoredCaptureSetup(const char* context, uint32_t timeout) {
        try {
            if (hasJpegConfig) {
                auto jpegJson = buildJpegJson(lastJpegConfig);
                log(LogLevel::Info, "%s: applying JPEG config %s", context, jpegJson.dump().c_str());
                auto r = syncCall(OP_SET_JPEG_CFGS, jpegJson, timeout);
                if (!r) {
                    log(LogLevel::Error, "%s: setJpegConfig failed: %s",
                        context, r.error().message.c_str());
                    return false;
                }
            }
            if (hasSubscription) {
                log(LogLevel::Info, "%s: applying subscription (triggerImage=%d, imgpkgActive=%d)",
                    context, lastSubscription.triggerImage, imgpkgActive);
                auto r = syncCall(OP_SET_CALLBACKS, buildCallbacksJson(lastSubscription), timeout);
                if (!r) {
                    log(LogLevel::Error, "%s: subscribe failed: %s",
                        context, r.error().message.c_str());
                    return false;
                }
            }
            if (hasTriggerConfig) {
                log(LogLevel::Info, "%s: applying trigger config", context);
                auto r = syncCall(OP_SET_EQUIP_CFGS,
                                  buildTriggerConfigJson(lastTriggerConfig, lastTriggerProfileId),
                                  timeout);
                if (!r) {
                    log(LogLevel::Error, "%s: setTriggerConfig failed: %s",
                        context, r.error().message.c_str());
                    return false;
                }
            }
            return true;
        } catch (const std::exception& ex) {
            log(LogLevel::Error, "%s: exception during capture setup: %s", context, ex.what());
            return false;
        } catch (...) {
            log(LogLevel::Error, "%s: unknown exception during capture setup", context);
            return false;
        }
    }

    // -----------------------------------------------------------------------
    // replaySession()  --  re-authenticate, re-subscribe, re-apply JPEG cfg
    // Returns true if all replay steps succeed.
    // -----------------------------------------------------------------------
    bool replaySession() {
        try {
            uint32_t timeout = defaultTimeout.load();

            log(LogLevel::Info, "Replaying session (auth=%d, jpeg=%d, subscribe=%d, trigger=%d)",
                hasPassword, hasJpegConfig, hasSubscription, hasTriggerConfig);

            if (hasPassword) {
                nlohmann::json params;
                params["pass"] = lastPassword;
                auto r = syncCallDirect(OP_AUTHENTICATE, params, timeout);
                if (!r) {
                    log(LogLevel::Error, "Reconnect: auth failed: %s", r.error().message.c_str());
                    return false;
                }
                if (!jsonBoolField(r.value(), "auth")) {
                    log(LogLevel::Error, "Reconnect: auth rejected");
                    return false;
                }
                isAuthenticated = true;
            }

            if (hasJpegConfig) {
                auto r = syncCallDirect(OP_SET_JPEG_CFGS, buildJpegJson(lastJpegConfig), timeout);
                if (!r)
                    log(isAuthenticated ? LogLevel::Error : LogLevel::Info,
                        "Reconnect: setJpegConfig failed: %s%s",
                        r.error().message.c_str(),
                        isAuthenticated ? "" : " (will be applied after authenticate())");
            }
            if (hasSubscription) {
                auto r = syncCallDirect(OP_SET_CALLBACKS, buildCallbacksJson(lastSubscription), timeout);
                if (!r)
                    log(isAuthenticated ? LogLevel::Error : LogLevel::Info,
                        "Reconnect: subscribe failed: %s%s",
                        r.error().message.c_str(),
                        isAuthenticated ? "" : " (will be applied after authenticate())");
            }
            if (hasTriggerConfig) {
                auto r = syncCallDirect(OP_SET_EQUIP_CFGS,
                                        buildTriggerConfigJson(lastTriggerConfig, lastTriggerProfileId),
                                        timeout);
                if (!r)
                    log(isAuthenticated ? LogLevel::Error : LogLevel::Info,
                        "Reconnect: setTriggerConfig failed: %s%s",
                        r.error().message.c_str(),
                        isAuthenticated ? "" : " (will be applied after authenticate())");
            }

            return true;
        } catch (const std::exception& ex) {
            log(LogLevel::Error, "Reconnect: exception during session replay: %s", ex.what());
            return false;
        } catch (...) {
            log(LogLevel::Error, "Reconnect: unknown exception during session replay");
            return false;
        }
    }

    // -----------------------------------------------------------------------
    // runReadLoop()  --  the select/read/ping loop, extracted for reuse
    // -----------------------------------------------------------------------
    void runReadLoop() {
        std::vector<uint8_t> rawBuf;
        size_t rawOffset = 0;
        uint8_t readBuf[TCP_BUF_SIZE];
        while (state.load() == RUNNING) {
            // Wait for data with 1 second timeout
            // socketWait returns: 1 = socket ready, 0 = timeout, -1 = error
            int waitResult = itscam_os::socketWait(sock, 1000, false);  // 1000ms, check read
            if (waitResult > 0) {
                int64_t n = itscam_os::socketRead(sock, readBuf, TCP_BUF_SIZE);
                if (n > 0) {
                    if (rawOffset == rawBuf.size()) {
                        rawBuf.clear();
                        rawOffset = 0;
                    } else if (rawOffset > 0 &&
                               (rawOffset >= rawBuf.size() / 2 || rawOffset >= TCP_BUF_SIZE)) {
                        compactRawBuffer(rawBuf, rawOffset);
                    }

                    rawBuf.insert(rawBuf.end(), readBuf, readBuf + n);
                    WireMsg wm;
                    while (readOneMsg(rawBuf, rawOffset, wm)) {
                        size_t qsz = inboundQueue.size();
                        if (qsz < 200) {
                            inboundQueue.push(wm);
                            if (qsz >= 150) {
                                log(LogLevel::Error,
                                    "Queue backpressure: %zu/200 (op=%u id=%u)",
                                    qsz + 1, (unsigned)wm.op, wm.id);
                            }
                        } else {
                            log(LogLevel::Error,
                                "Queue full, msg discarded (op=%u id=%u, queue=%zu)",
                                (unsigned)wm.op, wm.id, qsz);
                        }
                    }

                    if (rawOffset == rawBuf.size()) {
                        rawBuf.clear();
                        rawOffset = 0;
                    } else if (rawOffset > 0 &&
                               (rawOffset >= rawBuf.size() / 2 || rawOffset >= TCP_BUF_SIZE)) {
                        compactRawBuffer(rawBuf, rawOffset);
                    }
                } else if (n == 0) {
                    log(LogLevel::Error, "Socket disconnected");
                    state.store(STOPPING);
                } else {
                    int err = itscam_os::socketErrno();
                    if (err != ITSCAM_SOCK_EWOULDBLOCK) {
                        log(LogLevel::Error, "Socket error %d: %s", err, itscam_os::socketStrerror(err));
                        state.store(STOPPING);
                    }
                }
            }

            // Ping if idle
            uint64_t now = itscam_os::monotonicNow();
            uint64_t delta = now - lastSentTime;
            if (delta >= (uint64_t)pingInterval.load() * 1000) {
                sendPing();
            }

            // Note: unanswered ping detection is handled inside sendPing()
            // via the consecutivePingFailures counter.
        }
    }

    // -----------------------------------------------------------------------
    // cleanupAfterDisconnect()  --  fail pending promises, fire callbacks
    // -----------------------------------------------------------------------
    void cleanupAfterDisconnect() {
        closeSocket();

        if (handlerThread.joinable()) handlerThread.join();

        // Reset ping state and auth state so reconnection starts clean
        lastPingId.store(0);
        consecutivePingFailures = 0;
        isAuthenticated = false;

        log(LogLevel::Info, "Disconnected");

        // Fire disconnect callback
        {
            LockGuard<Mutex> lk(cbMtx);
            safeCallback("disconnect", cbDisconnect, std::string("disconnected"));
        }
        fireConnectionState(ConnectionState::Disconnected, "disconnected");

        // Fail all pending promises
        {
            LockGuard<Mutex> lk(promiseMtx);
            for (auto& kv : promises) {
                WireMsg fail;
                fail.op = OP_NACK;
                fail.id = kv.first;
                fail.body = jsonToBody({{"reason", "disconnected"}});
                kv.second.set_value(std::move(fail));
            }
            promises.clear();
        }
        // Fail pending capture exposure groups (wake waiters with partial data)
        {
            LockGuard<Mutex> lk(captureMtx);
            for (auto& kv : pendingCaptures) {
                kv.second->complete = true;  // wake with whatever we have
                kv.second->cv.notifyAll();
            }
            pendingCaptures.clear();
        }

        // Flush any partial exposure group accumulators for event callbacks
        {
            LockGuard<Mutex> lk(cbMtx);
            fireExposureGroupCallback(cbTriggerExposureGroup, triggerExpGroupAcc);
            fireExposureGroupCallback(cbSnapshotExposureGroup, snapshotExpGroupAcc);
        }
    }

    // -----------------------------------------------------------------------
    // Message handler thread
    // -----------------------------------------------------------------------
    void runHandlerThread() {
        while (state.load() == RUNNING || inboundQueue.size() > 0) {
            WireMsg wm;
            if (!inboundQueue.pop(wm, 100)) {
                // No message received; check for stale exposure groups
                try { checkExposureGroupTimeouts(); }
                catch (...) {} // user callback in fireExposureGroupCallback
                continue;
            }

            // Ping response
            if (wm.op == OP_NACK && wm.id == lastPingId.load()) {
                lastPingId.store(0);
                consecutivePingFailures = 0;
                continue;
            }

            try {
                switch (wm.op) {
                // --- JSON events ---
                case OP_SHUTDOWN:
                    state.store(STOPPING);
                    break;

                case OP_EVT_TRIGGER:
                    dispatchTriggerInfo(wm);
                    break;

                case OP_EVT_SNAPSHOT:
                    dispatchFrameInfo(wm, cbSnapshotMetadata);
                    break;

                case OP_EVT_PREVIEW:
                    dispatchFrameInfo(wm, cbPreviewMetadata);
                    break;

                case OP_EVT_PIPE_START:
                    dispatchFrameInfo(wm, cbPipelineStart);
                    break;

                case OP_EVT_GPIO:
                    dispatchGpio(wm);
                    break;

                case OP_EVT_SERIAL:
                    dispatchSerial(wm);
                    break;

                // --- Mixed (metadata + image) events ---
                case OP_JPEG_TRIGGER:
                case OP_IMGPKG_TRIGGER:
                    dispatchTriggerImage(wm);
                    break;

                case OP_JPEG_SNAPSHOT:
                case OP_IMGPKG_SNAPSHOT:
                    dispatchSnapshotImage(wm);
                    break;

                case OP_JPEG_PREVIEW:
                    dispatchImage(wm, cbPreviewImage);
                    break;

                // --- Responses ---
                default:
                    resolvePromise(wm);
                    break;
                }
            } catch (const std::exception& ex) {
                log(LogLevel::Error,
                    "Exception handling op %u: %s", (unsigned)wm.op, ex.what());
            } catch (...) {
                log(LogLevel::Error,
                    "Unknown exception handling op %u", (unsigned)wm.op);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Dispatch helpers
    // -----------------------------------------------------------------------
    /// Safe callback invocation helper -- catches exceptions from user code
    /// so that a bad callback never kills the handler thread.
    /// Note: callers typically hold cbMtx, so we cannot call log() here
    /// (it would deadlock). The handler thread's outer try-catch provides
    /// logging for any re-thrown exceptions.
    template<typename Fn, typename... Args>
    void safeCallback(const char* /*name*/, const Fn& fn, Args&&... args) {
        if (!fn) return;
        try {
            fn(std::forward<Args>(args)...);
        } catch (const std::exception&) {
            // Swallowed: cannot log here (cbMtx held).
            // The handler thread's outer catch will handle it if re-thrown.
        } catch (...) {
            // Swallowed: unknown callback exception.
        }
    }

    void dispatchFrameInfo(const WireMsg& wm,
                           const std::function<void(const FrameInfo&)>& cb) {
        LockGuard<Mutex> lk(cbMtx);
        if (!cb) return;
        nlohmann::json j = bodyToJson(wm.body);
        safeCallback("dispatchFrameInfo", cb, parseFrameInfo(j));
    }

    /// Specialized handler for OP_EVT_TRIGGER: parses the JSON, caches
    /// the detection metadata (plates/objects) by RID for later merging
    /// with IMGPKG events, then fires the user callback.
    void dispatchTriggerInfo(const WireMsg& wm) {
        LockGuard<Mutex> lk(cbMtx);
        nlohmann::json j = bodyToJson(wm.body);
        FrameInfo fi = parseFrameInfo(j);
        cacheTriggerInfo(fi);
        safeCallback("triggerMetadata", cbTriggerMetadata, fi);
    }

    void dispatchGpio(const WireMsg& wm) {
        LockGuard<Mutex> lk(cbMtx);
        if (!cbGpio) return;
        nlohmann::json j = bodyToJson(wm.body);
        GpioEvent ev;
        ev.frameCount  = j.value("framecount", (uint64_t)0);
        ev.risingEdge  = j.value("rising",     (uint32_t)0);
        ev.fallingEdge = j.value("falling",    (uint32_t)0);
        ev.state       = j.value("state",      (uint32_t)0);
        safeCallback("gpio", cbGpio, ev);
    }

    void dispatchSerial(const WireMsg& wm) {
        LockGuard<Mutex> lk(cbMtx);
        if (!cbSerial) return;
        if (wm.body.size() <= 4) return;
        uint32_t metaLen = itscam_os::netToHost32(*(const uint32_t*)wm.body.data());
        if (wm.body.size() < 4 + metaLen) return;
        std::vector<uint8_t> metaRaw(wm.body.begin() + 4,
                                      wm.body.begin() + 4 + metaLen);
        nlohmann::json meta = bodyToJson(metaRaw);
        SerialData sd;
        sd.port = strToSerialPort(jsonStrField(meta, "pipe", "serial1"));
        sd.data.assign(wm.body.begin() + 4 + metaLen, wm.body.end());
        sd.datetimeUtc = jsonStrField(meta, "DatetimeUTCMs");
        sd.uptimeMs = 0;
        if (meta.count("uptimeMs")) {
            if (meta["uptimeMs"].is_string()) {
                try { sd.uptimeMs = std::stoull(meta["uptimeMs"].get<std::string>()); }
                catch (...) {}
            } else if (meta["uptimeMs"].is_number()) {
                sd.uptimeMs = meta["uptimeMs"].get<uint64_t>();
            }
        }
        safeCallback("serial", cbSerial, sd);
    }

    void dispatchImage(const WireMsg& wm,
                       const std::function<void(const CaptureResult&)>& cb) {
        LockGuard<Mutex> lk(cbMtx);
        if (!cb) return;
        safeCallback("dispatchImage", cb, parseMixedBody(wm.body));
    }

    // -----------------------------------------------------------------------
    // accumulateExposureGroup -- accumulate frames into a group
    // Returns true if the group is now complete.
    // -----------------------------------------------------------------------
    bool accumulateExposureGroup(ExposureGroupAccumulator& acc,
                                const CaptureResult& cr) {
        int expLen = cr.info.multiExpLength;
        if (expLen <= 0) expLen = 1; // single-exposure frame

        // New group?
        if (acc.rid != cr.info.requestId || acc.rid == 0) {
            // Flush the old group if it had partial data
            // (caller is responsible for flushing before calling this)
            acc.rid = cr.info.requestId;
            acc.expectedCount = expLen;
            acc.frames.clear();
            acc.firstFrameTimeMs = itscam_os::monotonicNow();
        }

        acc.frames.push_back(cr);
        return (int)acc.frames.size() >= acc.expectedCount;
    }

    void fireExposureGroupCallback(
            const std::function<void(const std::vector<CaptureResult>&)>& cb,
            ExposureGroupAccumulator& acc) {
        if (!acc.frames.empty()) {
            safeCallback("exposureGroup", cb, acc.frames);
            acc.reset();
        }
    }

    // -----------------------------------------------------------------------
    // dispatchTriggerImage -- fire per-frame + accumulate exposure group
    // -----------------------------------------------------------------------
    void dispatchTriggerImage(const WireMsg& wm) {
        CaptureResult cr = parseMixedBody(wm.body);

        // Merge detection metadata from EVT_TRIGGER cache or JPEG comments
        // if the IMGPKG mixed body didn't include it (older firmware).
        std::string mergeDiag;
        {
            LockGuard<Mutex> lk(cbMtx);
            mergeDiag = mergeDetectionMetadata(cr);
        }
        // Log only successful merges (EVT_TRIGGER cache or JPEG COM).
        // The "no metadata" case fires on every frame and is too noisy.
        if (!mergeDiag.empty() && mergeDiag[0] != 'n') {
            log(LogLevel::Info, "RID %lu metadata: %s",
                (unsigned long)cr.info.requestId, mergeDiag.c_str());
        }

        // Fire the per-frame callback (isolated so an exception doesn't
        // prevent exposure group accumulation)
        {
            LockGuard<Mutex> lk(cbMtx);
            safeCallback("triggerImage", cbTriggerImage, cr);
        }

        // Accumulate for exposure group callback
        {
            LockGuard<Mutex> lk(cbMtx);
            if (!cbTriggerExposureGroup) return;

            // If a new RID arrives, flush any pending partial group
            if (triggerExpGroupAcc.rid != 0 &&
                triggerExpGroupAcc.rid != cr.info.requestId) {
                fireExposureGroupCallback(cbTriggerExposureGroup, triggerExpGroupAcc);
            }

            if (accumulateExposureGroup(triggerExpGroupAcc, cr)) {
                fireExposureGroupCallback(cbTriggerExposureGroup, triggerExpGroupAcc);
            }
        }
    }

    // -----------------------------------------------------------------------
    // dispatchSnapshotImage -- fire per-frame + accumulate exposure group
    // + resolve pending captureSnapshot futures.
    // -----------------------------------------------------------------------
    void dispatchSnapshotImage(const WireMsg& wm) {
        CaptureResult cr = parseMixedBody(wm.body);

        // Feed into pending captureSnapshot exposure groups
        {
            LockGuard<Mutex> lk(captureMtx);
            auto it = pendingCaptures.find(cr.info.requestId);
            if (it != pendingCaptures.end()) {
                auto& pg = it->second;
                int expLen = cr.info.multiExpLength;
                if (expLen <= 0) expLen = 1;
                if (pg->expectedCount == 0) pg->expectedCount = expLen;
                pg->frames.push_back(cr);
                if ((int)pg->frames.size() >= pg->expectedCount) {
                    pg->complete = true;
                    pg->cv.notifyAll();
                }
            }
        }

        // Fire the per-frame callback (isolated so an exception doesn't
        // prevent exposure group accumulation)
        {
            LockGuard<Mutex> lk(cbMtx);
            safeCallback("snapshotImage", cbSnapshotImage, cr);
        }

        // Accumulate for exposure group callback
        {
            LockGuard<Mutex> lk(cbMtx);
            if (!cbSnapshotExposureGroup) return;

            // If a new RID arrives, flush any pending partial group
            if (snapshotExpGroupAcc.rid != 0 &&
                snapshotExpGroupAcc.rid != cr.info.requestId) {
                fireExposureGroupCallback(cbSnapshotExposureGroup, snapshotExpGroupAcc);
            }

            if (accumulateExposureGroup(snapshotExpGroupAcc, cr)) {
                fireExposureGroupCallback(cbSnapshotExposureGroup, snapshotExpGroupAcc);
            }
        }
    }

    void resolvePromise(const WireMsg& wm) {
        LockGuard<Mutex> lk(promiseMtx);
        auto it = promises.find(wm.id);
        if (it != promises.end()) {
            it->second.set_value(wm);
        }
    }

    // -----------------------------------------------------------------------
    // checkExposureGroupTimeouts -- flush stale partial groups
    // -----------------------------------------------------------------------
    void checkExposureGroupTimeouts() {
        uint64_t now = itscam_os::monotonicNow();
        uint64_t timeout = exposureGroupTimeoutMs.load();
        LockGuard<Mutex> lk(cbMtx);

        if (triggerExpGroupAcc.rid != 0 &&
            (now - triggerExpGroupAcc.firstFrameTimeMs) > timeout) {
            fireExposureGroupCallback(cbTriggerExposureGroup, triggerExpGroupAcc);
        }
        if (snapshotExpGroupAcc.rid != 0 &&
            (now - snapshotExpGroupAcc.firstFrameTimeMs) > timeout) {
            fireExposureGroupCallback(cbSnapshotExposureGroup, snapshotExpGroupAcc);
        }
    }

    // -----------------------------------------------------------------------
    // Ping
    // -----------------------------------------------------------------------
    void sendPing() {
        uint32_t prevPingId = lastPingId.load();

        // If a previous ping is still in-flight (unanswered), count a failure
        if (prevPingId != 0) {
            ++consecutivePingFailures;
            uint32_t limit = maxPingFailures.load();
            if (limit > 0 && consecutivePingFailures >= limit) {
                log(LogLevel::Error,
                    "Ping timeout: %u consecutive unanswered pings "
                    "(limit %u), connection presumed dead",
                    consecutivePingFailures, limit);
                state.store(STOPPING);
                return;
            }
            // Clean up the stale ping promise before sending a new one
            {
                LockGuard<Mutex> lk(promiseMtx);
                promises.erase(prevPingId);
            }
            lastPingId.store(0);
        }

        uint32_t id = nextId();
        WireMsg m;
        m.op = OP_NACK;
        m.id = id;
        // empty body
        {
            LockGuard<Mutex> lk(promiseMtx);
            promises[id] = WirePromise<WireMsg>();
        }
        sendRaw(serializeMsg(m));
        lastPingId.store(id);
    }

    // -----------------------------------------------------------------------
    // Build SET_CALLBACKS JSON from EventSubscription
    // -----------------------------------------------------------------------
    nlohmann::json buildCallbacksJson(const EventSubscription& ev) {
        nlohmann::json j;
        j["pipeline"]       = ev.pipeline;
        j["trigger"]        = ev.triggerMetadata;
        j["snapshot"]       = ev.snapshotMetadata;
        j["preview"]        = ev.previewMetadata;
        j["gpio"]           = ev.gpio;

        // Map the unified "triggerImage" flag to either triggerjpeg or
        // triggerimgpkg depending on whether imgpkg options are active.
        if (imgpkgActive) {
            j["triggerimgpkg"]  = ev.triggerImage;
            j["triggerjpeg"]    = false;
            j["snapshotimgpkg"] = ev.snapshotImage;
            j["snapshotjpeg"]   = false;
        } else {
            j["triggerjpeg"]    = ev.triggerImage;
            j["triggerimgpkg"]  = false;
            j["snapshotjpeg"]   = ev.snapshotImage;
            j["snapshotimgpkg"] = false;
        }
        j["previewjpeg"]    = ev.previewImage;
        j["serial1"]        = ev.serial1;
        j["serial2"]        = ev.serial2;
        return j;
    }

    /// Read a JSON field as boolean, tolerating integer values (0/1)
    /// that older firmware may return instead of true/false.
    static bool jsonBool(const nlohmann::json& j, const char* key) {
        if (!j.count(key)) return false;
        auto& v = j[key];
        if (v.is_boolean()) return v.get<bool>();
        if (v.is_number())  return v.get<int>() != 0;
        return false;
    }

    EventSubscription parseSubscriptionResponse(const nlohmann::json& j) {
        EventSubscription ev;
        ev.pipeline         = jsonBool(j, "pipeline");
        ev.triggerMetadata   = jsonBool(j, "trigger");
        ev.snapshotMetadata  = jsonBool(j, "snapshot");
        ev.previewMetadata   = jsonBool(j, "preview");
        ev.gpio              = jsonBool(j, "gpio");
        ev.triggerImage      = jsonBool(j, "triggerjpeg")  || jsonBool(j, "triggerimgpkg");
        ev.snapshotImage     = jsonBool(j, "snapshotjpeg") || jsonBool(j, "snapshotimgpkg");
        ev.previewImage      = jsonBool(j, "previewjpeg");
        ev.serial1           = jsonBool(j, "serial1");
        ev.serial2           = jsonBool(j, "serial2");
        return ev;
    }

    // -----------------------------------------------------------------------
    // Build SET_JPEG_CFGS JSON from JpegConfig
    // -----------------------------------------------------------------------
    nlohmann::json buildJpegJson(const JpegConfig& cfg) {
        nlohmann::json j;
        if (cfg.triggerQuality >= 0)
            j["trigger"]["quality"] = cfg.triggerQuality;
        if (cfg.snapshotQuality >= 0)
            j["snapshot"]["quality"] = cfg.snapshotQuality;
        if (cfg.previewQuality >= 0)
            j["preview"]["quality"] = cfg.previewQuality;
        if (cfg.previewMinIntervalMs >= 0)
            j["preview"]["mindt"] = cfg.previewMinIntervalMs;
        if (cfg.imgpkg.embedExif >= 0)
            j["imgpkg"]["embedexif"] = (bool)cfg.imgpkg.embedExif;
        if (cfg.imgpkg.embedComments >= 0)
            j["imgpkg"]["embedcomments"] = (bool)cfg.imgpkg.embedComments;
        if (cfg.imgpkg.embedSignature >= 0)
            j["imgpkg"]["embedsign"] = (bool)cfg.imgpkg.embedSignature;
        return j;
    }

    JpegConfig parseJpegResponse(const nlohmann::json& j) {
        JpegConfig cfg;
        if (j.count("trigger"))
            cfg.triggerQuality = j["trigger"].value("quality", -1);
        if (j.count("snapshot"))
            cfg.snapshotQuality = j["snapshot"].value("quality", -1);
        if (j.count("preview")) {
            cfg.previewQuality = j["preview"].value("quality", -1);
            cfg.previewMinIntervalMs = j["preview"].value("mindt", -1);
        }
        if (j.count("imgpkg")) {
            cfg.imgpkg.embedExif      = j["imgpkg"].value("embedexif",     -1);
            cfg.imgpkg.embedComments  = j["imgpkg"].value("embedcomments", -1);
            cfg.imgpkg.embedSignature = j["imgpkg"].value("embedsign",     -1);
        }
        return cfg;
    }

    // -----------------------------------------------------------------------
    // Build TRIGGER_SNAPSHOT JSON from SnapshotRequest
    // -----------------------------------------------------------------------
    nlohmann::json buildSnapshotJson(const SnapshotRequest& req) {
        nlohmann::json j;
        if (!req.overlays.empty()) {
            j["stringMap"] = nlohmann::json::object();
            for (auto& kv : req.overlays) j["stringMap"][kv.first] = kv.second;
        }
        if (!req.multiExposure.empty()) {
            j["multExp"] = nlohmann::json::array();
            for (auto& step : req.multiExposure) {
                nlohmann::json exp;
                if (step.shutter.mode == ExposureParams::Absolute)
                    exp["shutter"]["value"] = step.shutter.value;
                else if (step.shutter.mode == ExposureParams::PercentOfCurrent)
                    exp["shutter"]["percent"] = (int)step.shutter.value;
                if (step.gain.mode == ExposureParams::Absolute)
                    exp["gain"]["value"] = step.gain.value;
                else if (step.gain.mode == ExposureParams::PercentOfCurrent)
                    exp["gain"]["percent"] = (int)step.gain.value;
                if (!step.flash.empty()) {
                    exp["flash"] = nlohmann::json::object();
                    for (auto& fl : step.flash)
                        exp["flash"][std::to_string(fl.channel)] = fl.powerPercent;
                }
                j["multExp"].push_back(exp);
            }
        }
        return j;
    }

    // -----------------------------------------------------------------------
    // Build / parse serial config JSON
    // -----------------------------------------------------------------------
    nlohmann::json buildSerialConfigJson(SerialPort port, const SerialConfig& cfg) {
        nlohmann::json inner;
        if (cfg.baudRate >= 0) inner["baud"]     = cfg.baudRate;
        if (cfg.dataBits >= 0) inner["databits"]  = cfg.dataBits;
        if (cfg.parity >= 0)   inner["parity"]    = cfg.parity;
        if (cfg.stopBits >= 0) inner["stopbits"]  = cfg.stopBits;
        nlohmann::json j;
        j[serialPortToStr(port)] = inner;
        return j;
    }

    SerialConfig parseSerialResponse(SerialPort port, const nlohmann::json& j) {
        SerialConfig cfg;
        std::string key = serialPortToStr(port);
        if (j.count(key) && j[key].is_object()) {
            auto& s = j[key];
            cfg.baudRate = s.value("baud",     -1);
            cfg.dataBits = s.value("databits", -1);
            cfg.parity   = s.value("parity",   -1);
            cfg.stopBits = s.value("stopbits", -1);
        }
        return cfg;
    }

    // -----------------------------------------------------------------------
    // Profile path helper
    // -----------------------------------------------------------------------
    static std::string profileBasePath(uint32_t profileId) {
        if (profileId == CURRENT_PROFILE)
            return "equip.currProfile";
        return "equip.profiles." + std::to_string(profileId);
    }

    // -----------------------------------------------------------------------
    // TriggerEvent <-> protocol string conversion
    // -----------------------------------------------------------------------
    static const char* triggerEventToStr(TriggerEvent ev) {
        switch (ev) {
            case TriggerEvent::Constant:    return "constant";
            case TriggerEvent::EdgeRising:  return "edge_rising";
            case TriggerEvent::EdgeFalling: return "edge_falling";
            case TriggerEvent::EdgeBoth:    return "edge_both";
            case TriggerEvent::LevelHigh:   return "level_high";
            case TriggerEvent::LevelLow:    return "level_low";
            case TriggerEvent::Motion:      return "motion";
            default:                        return "";
        }
    }

    static TriggerEvent strToTriggerEvent(const std::string& s) {
        if (s == "constant")     return TriggerEvent::Constant;
        if (s == "edge_rising")  return TriggerEvent::EdgeRising;
        if (s == "edge_falling") return TriggerEvent::EdgeFalling;
        if (s == "edge_both")    return TriggerEvent::EdgeBoth;
        if (s == "level_high")   return TriggerEvent::LevelHigh;
        if (s == "level_low")    return TriggerEvent::LevelLow;
        if (s == "motion")       return TriggerEvent::Motion;
        return TriggerEvent::Unchanged;
    }

    // -----------------------------------------------------------------------
    // Build / parse trigger config JSON
    // -----------------------------------------------------------------------
    nlohmann::json buildTriggerConfigJson(const TriggerConfig& cfg,
                                           uint32_t profileId) {
        nlohmann::json data;
        if (cfg.enabled >= 0)           data["trigger"]["enabled"]         = cfg.enabled;
        if (cfg.event != TriggerEvent::Unchanged) {
            data["trigger"]["event"] = triggerEventToStr(cfg.event);
        }
        if (cfg.port >= 0)              data["trigger"]["port"]            = cfg.port;
        if (cfg.minimumIntervalMs >= 0) data["trigger"]["minimumInterval"] = cfg.minimumIntervalMs;
        if (cfg.motionThreshold >= 0)   data["trigger"]["threshold"]       = cfg.motionThreshold;
        return nlohmann::json{{"path", profileBasePath(profileId)}, {"data", data}};
    }

    TriggerConfig parseTriggerResponse(const nlohmann::json& j) {
        TriggerConfig cfg;
        if (j.count("trigger") && j["trigger"].is_object()) {
            auto& t = j["trigger"];
            cfg.enabled           = t.value("enabled",         -1);
            cfg.event             = strToTriggerEvent(t.value("event", std::string()));
            cfg.port              = t.value("port",            -1);
            cfg.minimumIntervalMs = t.value("minimumInterval", -1);
            cfg.motionThreshold   = t.value("threshold",       -1);
        }
        return cfg;
    }

    // -----------------------------------------------------------------------
    // Build / parse multi-exposure config JSON
    // HAL format per step:
    //   {"shutter":{"value":N,"percentageOfCurrent":bool},
    //    "gain":{"value":N,"percentageOfCurrent":bool},
    //    "flash":{"power":[{"out":1,"percent":50},...]}}
    // -----------------------------------------------------------------------
    nlohmann::json buildMultiExpSettingsJson(const MultiExposureConfig& cfg) {
        nlohmann::json arr = nlohmann::json::array();
        for (auto& step : cfg.steps) {
            nlohmann::json entry;
            entry["shutter"]["value"] = step.shutterPercent;
            entry["shutter"]["percentageOfCurrent"] = true;
            entry["gain"]["value"] = step.gainPercent;
            entry["gain"]["percentageOfCurrent"] = true;

            nlohmann::json flashPower = nlohmann::json::array();
            for (auto& f : step.flash) {
                flashPower.push_back({{"out", f.gpioOutput},
                                      {"percent", f.powerPercent}});
            }
            entry["flash"]["power"] = flashPower;
            arr.push_back(entry);
        }
        return arr;
    }

    nlohmann::json buildMultiExpConfigJson(const MultiExposureConfig& cfg,
                                           uint32_t profileId) {
        nlohmann::json data;
        data["multiExps"]["enabled"] = cfg.enabled;
        data["multiExps"]["settings"] = buildMultiExpSettingsJson(cfg);
        return nlohmann::json{{"path", profileBasePath(profileId)}, {"data", data}};
    }

    MultiExposureConfig parseMultiExpResponse(const nlohmann::json& j) {
        MultiExposureConfig cfg;
        cfg.enabled = jsonBoolField(j, "enabled");
        if (j.count("settings") && j["settings"].is_array()) {
            for (auto& entry : j["settings"]) {
                MultiExpStep step;
                if (entry.count("shutter") && entry["shutter"].is_object()) {
                    step.shutterPercent = entry["shutter"].value("value", 100.0f);
                }
                if (entry.count("gain") && entry["gain"].is_object()) {
                    step.gainPercent = entry["gain"].value("value", 100.0f);
                }
                if (entry.count("flash") && entry["flash"].is_object() &&
                    entry["flash"].count("power") && entry["flash"]["power"].is_array()) {
                    for (auto& fp : entry["flash"]["power"]) {
                        MultiExpFlashOutput fo;
                        fo.gpioOutput   = fp.value("out", 1);
                        fo.powerPercent = fp.value("percent", 0);
                        step.flash.push_back(fo);
                    }
                }
                cfg.steps.push_back(step);
            }
        }
        return cfg;
    }

    // -----------------------------------------------------------------------
    // Build / parse exposure config JSON
    // -----------------------------------------------------------------------
    nlohmann::json buildExposureConfigJson(const ExposureConfig& cfg,
                                            uint32_t profileId) {
        nlohmann::json data;
        if (cfg.targetLevel >= 0)
            data["exposure"]["level"]["targetValue"] = cfg.targetLevel;
        if (cfg.shutter.automatic >= 0)
            data["exposure"]["shutter"]["automatic"] = cfg.shutter.automatic;
        if (cfg.shutter.fixedValue >= 0)
            data["exposure"]["shutter"]["fixedValue"] = cfg.shutter.fixedValue;
        if (cfg.shutter.minValue >= 0)
            data["exposure"]["shutter"]["minValue"] = cfg.shutter.minValue;
        if (cfg.shutter.maxValue >= 0)
            data["exposure"]["shutter"]["maxValue"] = cfg.shutter.maxValue;
        if (cfg.gain.automatic >= 0)
            data["exposure"]["gain"]["automatic"] = cfg.gain.automatic;
        if (cfg.gain.fixedValue >= 0)
            data["exposure"]["gain"]["fixedValue"] = cfg.gain.fixedValue;
        if (cfg.gain.minValue >= 0)
            data["exposure"]["gain"]["minValue"] = cfg.gain.minValue;
        if (cfg.gain.maxValue >= 0)
            data["exposure"]["gain"]["maxValue"] = cfg.gain.maxValue;
        if (cfg.iris.automatic >= 0)
            data["exposure"]["iris"]["automatic"] = cfg.iris.automatic;
        if (cfg.iris.fixedValue >= 0)
            data["exposure"]["iris"]["fixedValue"] = cfg.iris.fixedValue;
        return nlohmann::json{{"path", profileBasePath(profileId)}, {"data", data}};
    }

    ExposureConfig parseExposureResponse(const nlohmann::json& j) {
        ExposureConfig cfg;
        if (j.count("exposure") && j["exposure"].is_object()) {
            auto& e = j["exposure"];
            if (e.count("level"))
                cfg.targetLevel = e["level"].value("targetValue", -1);
            if (e.count("shutter")) {
                cfg.shutter.automatic  = e["shutter"].value("automatic",  -1);
                cfg.shutter.fixedValue = e["shutter"].value("fixedValue", -1);
                cfg.shutter.minValue   = e["shutter"].value("minValue",   -1);
                cfg.shutter.maxValue   = e["shutter"].value("maxValue",   -1);
            }
            if (e.count("gain")) {
                cfg.gain.automatic  = e["gain"].value("automatic",  -1);
                cfg.gain.fixedValue = e["gain"].value("fixedValue", -1);
                cfg.gain.minValue   = e["gain"].value("minValue",   -1);
                cfg.gain.maxValue   = e["gain"].value("maxValue",   -1);
            }
            if (e.count("iris")) {
                cfg.iris.automatic  = e["iris"].value("automatic",  -1);
                cfg.iris.fixedValue = e["iris"].value("fixedValue", -1);
            }
        }
        return cfg;
    }
};

//=========================================================================
// ItscamClient  --  public method implementations
//=========================================================================

ItscamClient::ItscamClient() : mImpl(new Impl) {}

ItscamClient::~ItscamClient() {
    if (mImpl) disconnect();
}

ItscamClient::ItscamClient(ItscamClient&&) noexcept = default;
ItscamClient& ItscamClient::operator=(ItscamClient&&) noexcept = default;

// ---- Connection -----------------------------------------------------------

Result<void> ItscamClient::connect(const std::string& address, uint16_t port,
                                    uint32_t timeoutMs,
                                    const AutoReconnectConfig& reconnect) {
    auto f = connectAsync(address, port, timeoutMs, reconnect);
    return f.get(timeoutMs);
}

Future<void> ItscamClient::connectAsync(const std::string& address,
                                         uint16_t port, uint32_t timeoutMs,
                                         const AutoReconnectConfig& reconnect) {
    if (mImpl->state.load() != Impl::IDLE && mImpl->state.load() != Impl::STOPPED) {
        Promise<void> p;
        p.set_value(Error{Error::InvalidParameter, "already connected or connecting"});
        return p.get_future();
    }

    mImpl->address = address;
    mImpl->port = port;
    mImpl->reconnectCfg = reconnect;
    mImpl->state.store(Impl::CONNECTING);

    auto impl = mImpl.get();
    auto pShared = std::make_shared<Promise<void>>();
    auto fut = pShared->get_future();

    // The socket thread runs the full read loop until disconnect.
    mImpl->socketThread = Thread([impl]() {
        try {
            impl->runSocketThread();
        } catch (const std::exception& ex) {
            impl->log(LogLevel::Error, "Socket thread exception: %s", ex.what());
            impl->state.store(Impl::STOPPED);
        } catch (...) {
            impl->log(LogLevel::Error, "Socket thread unknown exception");
            impl->state.store(Impl::STOPPED);
        }
    });
    mImpl->socketThread.detach();

    // Poll in a background thread for the connect state transition
    Thread([impl, pShared, timeoutMs]() {
        try {
            auto start = itscam_os::monotonicNow();
            while (true) {
                int st = impl->state.load();
                if (st == Impl::RUNNING) {
                    pShared->set_value(Result<void>());
                    return;
                }
                if (st == Impl::STOPPED) {
                    pShared->set_value(Error{Error::ConnectionFailed, "connection failed"});
                    return;
                }
                auto elapsed = itscam_os::monotonicNow() - start;
                if (elapsed >= timeoutMs) {
                    impl->state.store(Impl::STOPPING);
                    pShared->set_value(Error{Error::Timeout, "connect timed out"});
                    return;
                }
                itscam_os::sleepForMs(10);
            }
        } catch (...) {
            try { pShared->set_value(Error{Error::Unknown, "exception in connect poll"}); }
            catch (...) {}
        }
    }).detach();

    return fut;
}

void ItscamClient::disconnect() {
    if (!mImpl) return;
    int st = mImpl->state.load();
    if (st == Impl::RUNNING || st == Impl::CONNECTING ||
        st == Impl::STOPPING) {
        // DISCONNECTING tells the reconnect loop to stop
        mImpl->state.store(Impl::DISCONNECTING);
    }
    // Give threads some time to wind down
    for (int i = 0; i < 100 && mImpl->state.load() != Impl::STOPPED &&
                     mImpl->state.load() != Impl::IDLE; ++i) {
        itscam_os::sleepForMs(20);
    }
    mImpl->closeSocket();
}

bool ItscamClient::isConnected() const {
    return mImpl && mImpl->state.load() == Impl::RUNNING;
}

// ---- Authentication -------------------------------------------------------

void ItscamClient::setPassword(const std::string& password) {
    mImpl->lastPassword = password;
    mImpl->hasPassword  = true;
}

Result<void> ItscamClient::authenticate(const std::string& password,
                                         uint32_t timeoutMs) {
    try {
        nlohmann::json params;
        params["pass"] = password;
        auto r = mImpl->syncCall(OP_AUTHENTICATE, params, timeoutMs);
        if (!r) return r.error();
        auto& resp = r.value();
        if (jsonBoolField(resp, "auth")) {
            // Record for session replay on reconnect
            mImpl->lastPassword    = password;
            mImpl->hasPassword     = true;
            mImpl->isAuthenticated = true;

            // Apply any capture setup that was deferred until after auth.
            if (!mImpl->applyStoredCaptureSetup("Authenticate", timeoutMs)) {
                mImpl->log(LogLevel::Error,
                           "Authenticate: pending capture setup was not fully applied");
            }
            return Result<void>();
        }
        return Error{Error::NotAuthenticated,
                     resp.value("msg", "authentication failed")};
    } catch (const std::exception& ex) {
        return Error{Error::Unknown, std::string("auth exception: ") + ex.what()};
    } catch (...) {
        return Error{Error::Unknown, "auth unknown exception"};
    }
}

Future<void> ItscamClient::authenticateAsync(const std::string& password,
                                              uint32_t timeoutMs) {
    nlohmann::json params;
    params["pass"] = password;
    auto jsonFut = mImpl->asyncCall(OP_AUTHENTICATE, params, timeoutMs);

    auto pShared = std::make_shared<Promise<void>>();
    auto fut = pShared->get_future();
    auto* impl = mImpl.get();
    Thread([jf = std::move(jsonFut), pShared, impl, password, timeoutMs]() mutable {
        try {
            auto r = jf.get();
            if (!r) { pShared->set_value(r.error()); return; }
            auto& resp = r.value();
            if (jsonBoolField(resp, "auth")) {
                impl->lastPassword    = password;
                impl->hasPassword     = true;
                impl->isAuthenticated = true;
                if (!impl->applyStoredCaptureSetup("AuthenticateAsync", timeoutMs)) {
                    impl->log(LogLevel::Error,
                              "AuthenticateAsync: pending capture setup was not fully applied");
                }
                pShared->set_value(Result<void>());
            } else {
                pShared->set_value(Error{Error::NotAuthenticated,
                                   resp.value("msg", "authentication failed")});
            }
        } catch (const std::exception& ex) {
            try { pShared->set_value(Error{Error::Unknown, std::string("exception: ") + ex.what()}); }
            catch (...) {}
        } catch (...) {
            try { pShared->set_value(Error{Error::Unknown, "unknown exception"}); }
            catch (...) {}
        }
    }).detach();
    return fut;
}

// ---- Event subscriptions --------------------------------------------------

Result<EventSubscription> ItscamClient::subscribe(
    const EventSubscription& events, uint32_t timeoutMs) {
    try {
        // Record for session replay on reconnect BEFORE the call.
        // This ensures the subscription is stored even if the call fails
        // due to disconnection, allowing replay when connection is established.
        mImpl->lastSubscription = events;
        mImpl->hasSubscription  = true;

        // Defer if auth is known to be required but not yet done this session.
        if (mImpl->pendingAuth()) {
            mImpl->log(LogLevel::Info, "Subscription deferred until after authenticate()");
            return events;
        }

        auto r = mImpl->syncCall(OP_SET_CALLBACKS,
                                  mImpl->buildCallbacksJson(events), timeoutMs);
        if (!r) {
            const Error& err = r.error();
            if ((err.code == Error::ConnectionFailed || err.code == Error::Disconnected) &&
                mImpl->canQueueForReplay()) {
                mImpl->log(LogLevel::Info,
                           "Event subscription queued for replay after reconnect");
                return events;
            }
            return err;
        }
        return mImpl->parseSubscriptionResponse(r.value());
    } catch (const std::exception& ex) {
        return Error{Error::Unknown, std::string("subscribe exception: ") + ex.what()};
    } catch (...) {
        return Error{Error::Unknown, "subscribe unknown exception"};
    }
}

Result<EventSubscription> ItscamClient::subscribeCaptures(
    const CaptureSubscriptionConfig& config, uint32_t timeoutMs) {
    JpegConfig jpegConfig = config.toJpegConfig();
    EventSubscription events = config.toEventSubscription();

    // Preserve the intended capture setup even if the socket is currently
    // disconnected. This keeps replaySession() able to restore both parts
    // once the first connection or a later reconnect succeeds.
    mImpl->imgpkgActive = jpegConfig.usesImgpkg();
    mImpl->lastJpegConfig = jpegConfig;
    mImpl->hasJpegConfig = true;
    mImpl->lastSubscription = events;
    mImpl->hasSubscription = true;

    auto jpegResult = setJpegConfig(jpegConfig, timeoutMs);
    if (!jpegResult) {
        const Error& err = jpegResult.error();
        if ((err.code == Error::ConnectionFailed || err.code == Error::Disconnected) &&
            mImpl->canQueueForReplay()) {
            mImpl->log(LogLevel::Info,
                       "Capture setup queued for replay after reconnect");
            return events;
        }
        return err;
    }

    auto subResult = subscribe(events, timeoutMs);
    if (!subResult) {
        const Error& err = subResult.error();
        if ((err.code == Error::ConnectionFailed || err.code == Error::Disconnected) &&
            mImpl->canQueueForReplay()) {
            mImpl->log(LogLevel::Info,
                       "Capture setup queued for replay after reconnect");
            return events;
        }
        return err;
    }
    return subResult;
}

// ---- Image configuration --------------------------------------------------

Result<JpegConfig> ItscamClient::setJpegConfig(const JpegConfig& config,
                                                uint32_t timeoutMs) {
    try {
        // Track whether imgpkg is being activated for subscribe() mapping
        mImpl->imgpkgActive = config.usesImgpkg();

        // Record for session replay on reconnect BEFORE the call.
        // This ensures the config is stored even if the call fails
        // due to disconnection, allowing replay when connection is established.
        mImpl->lastJpegConfig = config;
        mImpl->hasJpegConfig  = true;

        // Defer if auth is known to be required but not yet done this session.
        if (mImpl->pendingAuth()) {
            mImpl->log(LogLevel::Info, "JPEG config deferred until after authenticate()");
            return config;
        }

        auto r = mImpl->syncCall(OP_SET_JPEG_CFGS,
                                  mImpl->buildJpegJson(config), timeoutMs);
        if (!r) {
            const Error& err = r.error();
            if ((err.code == Error::ConnectionFailed || err.code == Error::Disconnected) &&
                mImpl->canQueueForReplay()) {
                mImpl->log(LogLevel::Info,
                           "JPEG config queued for replay after reconnect");
                return config;
            }
            return err;
        }
        return mImpl->parseJpegResponse(r.value());
    } catch (const std::exception& ex) {
        return Error{Error::Unknown, std::string("setJpegConfig exception: ") + ex.what()};
    } catch (...) {
        return Error{Error::Unknown, "setJpegConfig unknown exception"};
    }
}

// ---- Capture --------------------------------------------------------------

Result<std::vector<CaptureResult>> ItscamClient::captureSnapshot(
    const SnapshotRequest& request, uint32_t timeoutMs) {
    auto f = captureSnapshotAsync(request, timeoutMs);
    return f.get(timeoutMs);
}

Future<std::vector<CaptureResult>> ItscamClient::captureSnapshotAsync(
    const SnapshotRequest& request, uint32_t timeoutMs) {

    // Step 1: send TRIGGER_SNAPSHOT
    uint32_t msgId;
    auto params = mImpl->buildSnapshotJson(request);
    if (!mImpl->sendRequest(OP_TRIGGER_SNAPSHOT, params, msgId)) {
        auto pShared = std::make_shared<Promise<std::vector<CaptureResult>>>();
        pShared->set_value(Error{Error::ConnectionFailed, "failed to send"});
        return pShared->get_future();
    }

    // Step 2: wait for the TRIGGER_SNAPSHOT response to get the RID
    auto* impl = mImpl.get();
    auto pShared = std::make_shared<Promise<std::vector<CaptureResult>>>();
    auto outerFut = pShared->get_future();

    Thread([impl, msgId, timeoutMs, pShared]() mutable {
        try {
            auto respResult = impl->waitResponse(OP_TRIGGER_SNAPSHOT, msgId, timeoutMs);
            if (!respResult) {
                pShared->set_value(respResult.error());
                return;
            }
            uint64_t rid = respResult.value().value("rid", (uint64_t)0);
            if (rid == 0) {
                pShared->set_value(Error{Error::ServerError, "no RID in response"});
                return;
            }

            // Step 3: register a PendingCaptureExposureGroup for this RID
            auto pg = std::make_shared<Impl::PendingCaptureExposureGroup>();
            {
                LockGuard<Mutex> lk(impl->captureMtx);
                impl->pendingCaptures[rid] = pg;
            }

            // Step 4: wait for all exposure group frames to arrive
            {
                UniqueLock<Mutex> lk(impl->captureMtx);
                bool ok = pg->cv.waitFor(lk, timeoutMs,
                                           [&]{ return pg->complete; });
                std::vector<CaptureResult> result = std::move(pg->frames);
                impl->pendingCaptures.erase(rid);
                lk.unlock();

                if (result.empty() && !ok) {
                    pShared->set_value(Error{Error::Timeout,
                                      "capture timed out waiting for image"});
                } else {
                    pShared->set_value(std::move(result));
                }
            }
        } catch (const std::exception& ex) {
            try { pShared->set_value(Error{Error::Unknown, std::string("exception: ") + ex.what()}); }
            catch (...) {}
        } catch (...) {
            try { pShared->set_value(Error{Error::Unknown, "unknown exception"}); }
            catch (...) {}
        }
    }).detach();

    return outerFut;
}

Result<std::vector<uint8_t>> ItscamClient::getLastFrame(int quality,
                                                         uint32_t timeoutMs) {
    auto f = getLastFrameAsync(quality, timeoutMs);
    return f.get(timeoutMs);
}

Future<std::vector<uint8_t>> ItscamClient::getLastFrameAsync(int quality,
                                                              uint32_t timeoutMs) {
    uint32_t id;
    nlohmann::json params;
    params["quality"] = quality;
    if (!mImpl->sendRequest(OP_GET_LASTFRAME, params, id)) {
        auto pShared = std::make_shared<Promise<std::vector<uint8_t>>>();
        pShared->set_value(Error{Error::ConnectionFailed, "failed to send"});
        return pShared->get_future();
    }
    auto* impl = mImpl.get();
    auto pShared = std::make_shared<Promise<std::vector<uint8_t>>>();
    auto outerFut = pShared->get_future();
    Thread([impl, id, timeoutMs, pShared]() mutable {
        try {
            // GET_LASTFRAME response body IS the JPEG data (no JSON wrapper)
            WireFuture<WireMsg> fut;
            {
                LockGuard<Mutex> lk(impl->promiseMtx);
                auto it = impl->promises.find(id);
                if (it == impl->promises.end()) {
                    pShared->set_value(Error{Error::Unknown, "promise not found"});
                    return;
                }
                fut = it->second.get_future();
            }
            if (fut.waitFor(timeoutMs) == WireFuture<WireMsg>::Status::timeout) {
                LockGuard<Mutex> lk(impl->promiseMtx);
                impl->promises.erase(id);
                pShared->set_value(Error{Error::Timeout, "request timed out"});
                return;
            }
            WireMsg resp = fut.get();
            {
                LockGuard<Mutex> lk(impl->promiseMtx);
                impl->promises.erase(id);
            }
            if (resp.op == OP_NACK) {
                nlohmann::json nack = bodyToJson(resp.body);
                pShared->set_value(Error{Error::ServerError,
                                  nack.value("reason", "failed to get frame")});
                return;
            }
            pShared->set_value(std::move(resp.body));
        } catch (const std::exception& ex) {
            try { pShared->set_value(Error{Error::Unknown, std::string("exception: ") + ex.what()}); }
            catch (...) {}
        } catch (...) {
            try { pShared->set_value(Error{Error::Unknown, "unknown exception"}); }
            catch (...) {}
        }
    }).detach();
    return outerFut;
}

// ---- Profile management ---------------------------------------------------

Result<uint32_t> ItscamClient::getActiveProfileId(uint32_t timeoutMs) {
    try {
        nlohmann::json params = {{"path", "equip.currProfile.id"}};
        auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
        if (!r) return r.error();
        auto& j = r.value();
        if (j.is_number()) return j.get<uint32_t>();
        return Error{Error::ServerError, "unexpected response for profile id"};
    } catch (const std::exception& ex) {
        return Error{Error::Unknown, std::string("getActiveProfileId exception: ") + ex.what()};
    } catch (...) {
        return Error{Error::Unknown, "getActiveProfileId unknown exception"};
    }
}

Result<void> ItscamClient::setActiveProfile(uint32_t profileId,
                                             uint32_t timeoutMs) {
    nlohmann::json params = {
        {"path", "equip.currProfile"},
        {"data", {{"id", profileId}}}
    };
    auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
    if (!r) return r.error();
    return Result<void>();
}

Result<std::vector<ProfileInfo>> ItscamClient::listProfiles(uint32_t timeoutMs) {
    try {
        // Step 1: read all profiles
        nlohmann::json params = {{"path", "equip.profiles"}};
        auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
        if (!r) return r.error();

        // Step 2: get the current profile ID to set the active flag
        uint32_t activeId = 0;
        auto ar = getActiveProfileId(timeoutMs);
        if (ar) activeId = ar.value();

        // Step 3: parse the response  (keys are profile IDs)
        std::vector<ProfileInfo> profiles;
        auto& j = r.value();
        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                ProfileInfo pi;
                try { pi.id = std::stoul(it.key()); }
                catch (...) { continue; }
                if (it.value().is_object()) {
                    pi.name        = it.value().value("name",        "");
                    pi.description = it.value().value("description", "");
                }
                pi.active = (pi.id == activeId);
                profiles.push_back(std::move(pi));
            }
        }
        return profiles;
    } catch (const std::exception& ex) {
        return Error{Error::Unknown, std::string("listProfiles exception: ") + ex.what()};
    } catch (...) {
        return Error{Error::Unknown, "listProfiles unknown exception"};
    }
}

// ---- Trigger & Exposure ---------------------------------------------------

Result<TriggerConfig> ItscamClient::getTriggerConfig(uint32_t profileId,
                                                      uint32_t timeoutMs) {
    try {
        std::string base = Impl::profileBasePath(profileId);
        nlohmann::json params = {{"path", base + ".trigger"}};
        auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
        if (!r) return r.error();
        nlohmann::json wrapper;
        wrapper["trigger"] = r.value();
        return mImpl->parseTriggerResponse(wrapper);
    } catch (const std::exception& ex) {
        return Error{Error::Unknown, std::string("getTriggerConfig exception: ") + ex.what()};
    } catch (...) {
        return Error{Error::Unknown, "getTriggerConfig unknown exception"};
    }
}

Result<void> ItscamClient::setTriggerConfig(const TriggerConfig& config,
                                             uint32_t profileId,
                                             uint32_t timeoutMs) {
    // Store for session replay on reconnect BEFORE the call.
    mImpl->lastTriggerConfig    = config;
    mImpl->lastTriggerProfileId = profileId;
    mImpl->hasTriggerConfig     = true;

    // Defer if auth is known to be required but not yet done this session.
    if (mImpl->pendingAuth()) {
        mImpl->log(LogLevel::Info, "Trigger config deferred until after authenticate()");
        return Result<void>();
    }

    auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS,
                              mImpl->buildTriggerConfigJson(config, profileId),
                              timeoutMs);
    if (!r) {
        const Error& err = r.error();
        if ((err.code == Error::ConnectionFailed || err.code == Error::Disconnected) &&
            mImpl->canQueueForReplay()) {
            mImpl->log(LogLevel::Info, "Trigger config queued for replay after reconnect");
            return Result<void>();
        }
        return err;
    }
    return Result<void>();
}

Result<ExposureConfig> ItscamClient::getExposureConfig(uint32_t profileId,
                                                        uint32_t timeoutMs) {
    try {
        std::string base = Impl::profileBasePath(profileId);
        nlohmann::json params = {{"path", base + ".exposure"}};
        auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
        if (!r) return r.error();
        nlohmann::json wrapper;
        wrapper["exposure"] = r.value();
        return mImpl->parseExposureResponse(wrapper);
    } catch (const std::exception& ex) {
        return Error{Error::Unknown, std::string("getExposureConfig exception: ") + ex.what()};
    } catch (...) {
        return Error{Error::Unknown, "getExposureConfig unknown exception"};
    }
}

Result<void> ItscamClient::setExposureConfig(const ExposureConfig& config,
                                              uint32_t profileId,
                                              uint32_t timeoutMs) {
    auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS,
                              mImpl->buildExposureConfigJson(config, profileId),
                              timeoutMs);
    if (!r) return r.error();
    return Result<void>();
}

// ---- Multi-exposure configuration -----------------------------------------

Result<MultiExposureConfig> ItscamClient::getMultiExposureConfig(
    uint32_t profileId, uint32_t timeoutMs) {
    try {
        std::string base = Impl::profileBasePath(profileId);
        nlohmann::json params = {{"path", base + ".multiExps"}};
        auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
        if (!r) return r.error();
        return mImpl->parseMultiExpResponse(r.value());
    } catch (const std::exception& ex) {
        return Error{Error::Unknown, std::string("getMultiExposureConfig exception: ") + ex.what()};
    } catch (...) {
        return Error{Error::Unknown, "getMultiExposureConfig unknown exception"};
    }
}

Result<void> ItscamClient::setMultiExposureConfig(
    const MultiExposureConfig& config, uint32_t profileId,
    uint32_t timeoutMs) {
    auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS,
                              mImpl->buildMultiExpConfigJson(config, profileId),
                              timeoutMs);
    if (!r) return r.error();
    return Result<void>();
}

// ---- Serial communication -------------------------------------------------

Result<SerialConfig> ItscamClient::configureSerial(SerialPort port,
                                                    const SerialConfig& config,
                                                    uint32_t timeoutMs) {
    try {
        auto r = mImpl->syncCall(OP_SET_SERIAL_CFGS,
                                  mImpl->buildSerialConfigJson(port, config),
                                  timeoutMs);
        if (!r) return r.error();
        return mImpl->parseSerialResponse(port, r.value());
    } catch (const std::exception& ex) {
        return Error{Error::Unknown, std::string("configureSerial exception: ") + ex.what()};
    } catch (...) {
        return Error{Error::Unknown, "configureSerial unknown exception"};
    }
}

Result<int> ItscamClient::sendSerialAscii(SerialPort port,
                                           const std::string& data,
                                           uint32_t timeoutMs) {
    nlohmann::json params;
    params["pipe"] = serialPortToStr(port);
    params["ascii"] = data;
    auto r = mImpl->syncCall(OP_SEND_SERIAL_DATA, params, timeoutMs);
    if (!r) return r.error();
    return r.value().value("len", 0);
}

Result<int> ItscamClient::sendSerialHex(SerialPort port,
                                         const std::string& hexData,
                                         uint32_t timeoutMs) {
    nlohmann::json params;
    params["pipe"] = serialPortToStr(port);
    params["hex"] = hexData;
    auto r = mImpl->syncCall(OP_SEND_SERIAL_DATA, params, timeoutMs);
    if (!r) return r.error();
    return r.value().value("len", 0);
}

Result<int> ItscamClient::sendSerialBase64(SerialPort port,
                                            const std::string& b64Data,
                                            uint32_t timeoutMs) {
    nlohmann::json params;
    params["pipe"] = serialPortToStr(port);
    params["base64"] = b64Data;
    auto r = mImpl->syncCall(OP_SEND_SERIAL_DATA, params, timeoutMs);
    if (!r) return r.error();
    return r.value().value("len", 0);
}

// ---- Typed equipment configuration ----------------------------------------

Result<DeviceInfo> ItscamClient::getDeviceInfo(uint32_t timeoutMs) {
    try {
        nlohmann::json params = {{"path", "equip.miscRO"}};
        auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
        if (!r) return r.error();
        auto& j = r.value();
        DeviceInfo info;
        info.cameraModel      = jsonStrField(j, "cameraModel");
        info.firmwareVersion   = jsonStrField(j, "firmwareVersion");
        info.imageSize.width  = j.value("imageWidth",  0);
        info.imageSize.height = j.value("imageHeight", 0);
        info.sensorType       = jsonStrField(j, "sensorType");
        info.lensModel        = jsonStrField(j, "lensModel");
        info.hdrAvailable     = jsonBoolField(j, "hdrAvailable");
        info.framePeriodUs    = j.value("framePeriodUs", 0);
        return info;
    } catch (const std::exception& ex) {
        return Error{Error::Unknown, std::string("getDeviceInfo exception: ") + ex.what()};
    } catch (...) {
        return Error{Error::Unknown, "getDeviceInfo unknown exception"};
    }
}

Result<void> ItscamClient::setScenarioOverlay(int scenario,
                                               const std::string& overlayText,
                                               uint32_t timeoutMs) {
    std::string key = "scenario" + std::to_string(scenario) + "Overlay";
    nlohmann::json params = {
        {"path", "equip.misc"},
        {"data", {{key, overlayText}}}
    };
    auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
    if (!r) return r.error();
    return Result<void>();
}

Result<void> ItscamClient::setScenarioCrop(int scenario,
                                            const ScenarioCrop& crop,
                                            uint32_t timeoutMs) {
    std::string key = "scenario" + std::to_string(scenario) + "Crop";
    nlohmann::json cropJson = {{"x0", crop.x0}, {"y0", crop.y0},
                                {"x1", crop.x1}, {"y1", crop.y1}};
    nlohmann::json params = {
        {"path", "equip.misc"},
        {"data", {{key, cropJson}}}
    };
    auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
    if (!r) return r.error();
    return Result<void>();
}

Result<void> ItscamClient::setSnapshotCrop(bool enable,
                                            const ScenarioCrop& crop,
                                            uint32_t timeoutMs) {
    nlohmann::json data;
    data["snapshotCropEn"] = enable;
    data["snapshotCrop"] = {{"x0", crop.x0}, {"y0", crop.y0},
                             {"x1", crop.x1}, {"y1", crop.y1}};
    nlohmann::json params = {{"path", "equip.misc"}, {"data", data}};
    auto r = mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
    if (!r) return r.error();
    return Result<void>();
}

// ---- Generic equipment configuration -------------------------------------

Result<nlohmann::json> ItscamClient::getAllConfigs(uint32_t timeoutMs) {
    return getConfig("", timeoutMs);
}

Result<nlohmann::json> ItscamClient::getConfig(const std::string& path,
                                                uint32_t timeoutMs) {
    nlohmann::json params;
    if (!path.empty()) params["path"] = path;
    return mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
}

Result<nlohmann::json> ItscamClient::setConfig(const std::string& path,
                                                const nlohmann::json& data,
                                                uint32_t timeoutMs) {
    nlohmann::json params;
    params["path"] = path;
    params["data"] = data;
    return mImpl->syncCall(OP_SET_EQUIP_CFGS, params, timeoutMs);
}

// ---- System ---------------------------------------------------------------

Result<void> ItscamClient::reboot(uint32_t timeoutMs) {
    auto r = mImpl->syncCall(OP_CMD_REBOOT, nlohmann::json::object(), timeoutMs);
    if (!r) return r.error();
    return Result<void>();
}

// ---- Event callbacks ------------------------------------------------------

void ItscamClient::onTriggerMetadata(std::function<void(const FrameInfo&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbTriggerMetadata = std::move(cb);
}
void ItscamClient::onSnapshotMetadata(std::function<void(const FrameInfo&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbSnapshotMetadata = std::move(cb);
}
void ItscamClient::onPreviewMetadata(std::function<void(const FrameInfo&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbPreviewMetadata = std::move(cb);
}
void ItscamClient::onTriggerImage(std::function<void(const CaptureResult&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbTriggerImage = std::move(cb);
}
void ItscamClient::onSnapshotImage(std::function<void(const CaptureResult&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbSnapshotImage = std::move(cb);
}
void ItscamClient::onPreviewImage(std::function<void(const CaptureResult&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbPreviewImage = std::move(cb);
}
void ItscamClient::onPipelineStart(std::function<void(const FrameInfo&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbPipelineStart = std::move(cb);
}
void ItscamClient::onGpio(std::function<void(const GpioEvent&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbGpio = std::move(cb);
}
void ItscamClient::onSerial(std::function<void(const SerialData&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbSerial = std::move(cb);
}
void ItscamClient::onTriggerExposureGroup(
    std::function<void(const std::vector<CaptureResult>&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbTriggerExposureGroup = std::move(cb);
}
void ItscamClient::onSnapshotExposureGroup(
    std::function<void(const std::vector<CaptureResult>&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbSnapshotExposureGroup = std::move(cb);
}
void ItscamClient::onDisconnect(std::function<void(const std::string&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbDisconnect = std::move(cb);
}

void ItscamClient::onConnectionStateChanged(
    std::function<void(ConnectionState, const std::string&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbConnectionState = std::move(cb);
}

// ---- Advanced -------------------------------------------------------------

void ItscamClient::setPingInterval(uint32_t seconds) {
    mImpl->pingInterval.store(seconds);
}

void ItscamClient::setMaxPingFailures(uint32_t count) {
    mImpl->maxPingFailures.store(count);
}

void ItscamClient::setLogHandler(
    std::function<void(LogLevel, const std::string&)> cb) {
    LockGuard<Mutex> lk(mImpl->cbMtx);
    mImpl->cbLog = std::move(cb);
}

void ItscamClient::setDefaultTimeout(uint32_t milliseconds) {
    mImpl->defaultTimeout.store(milliseconds);
}

void ItscamClient::setExposureGroupTimeout(uint32_t milliseconds) {
    mImpl->exposureGroupTimeoutMs.store(milliseconds);
}

// ---- Low-level ------------------------------------------------------------

Result<nlohmann::json> ItscamClient::rawCall(uint16_t opcode,
                                              const nlohmann::json& params,
                                              uint32_t timeoutMs) {
    return mImpl->syncCall(opcode, params, timeoutMs);
}

Future<nlohmann::json> ItscamClient::rawCallAsync(uint16_t opcode,
                                                    const nlohmann::json& params,
                                                    uint32_t timeoutMs) {
    return mImpl->asyncCall(opcode, params, timeoutMs);
}

} // namespace itscam

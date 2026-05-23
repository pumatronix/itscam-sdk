/*
 *  itscam_client.h
 *
 *  ITSCAM Client SDK - Main client class
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Requires: nlohmann/json 3.x (header-only).
 */
#pragma once

#include "itscam_types.h"

#ifndef INCLUDE_NLOHMANN_JSON_HPP_
#include "3rdparty/nlohmann/json.hpp"
#endif

#include <functional>
#include <memory>
#include <string>

namespace itscam {

class ItscamClient {
public:
    ItscamClient();
    ~ItscamClient();

    // Move-only (PIMPL)
    ItscamClient(ItscamClient&&) noexcept;
    ItscamClient& operator=(ItscamClient&&) noexcept;
    ItscamClient(const ItscamClient&) = delete;
    ItscamClient& operator=(const ItscamClient&) = delete;

    // =========================================================================
    //  Connection
    // =========================================================================

    Result<void> connect(const std::string& address,
                         uint16_t port = 60000,
                         uint32_t timeoutMs = 5000,
                         const AutoReconnectConfig& reconnect = {});

    Future<void> connectAsync(const std::string& address,
                              uint16_t port = 60000,
                              uint32_t timeoutMs = 5000,
                              const AutoReconnectConfig& reconnect = {});

    void disconnect();
    bool isConnected() const;

    // =========================================================================
    //  Authentication
    // =========================================================================

    /// Pre-store credentials so that replaySession() authenticates before
    /// sending any other commands. Call this before connect() when the
    /// password is known upfront. On reconnect the stored password is also
    /// used automatically.
    void setPassword(const std::string& password);

    Result<void> authenticate(const std::string& password,
                              uint32_t timeoutMs = 10000);

    Future<void> authenticateAsync(const std::string& password,
                                   uint32_t timeoutMs = 10000);

    /// Common-case helper for capture subscriptions.
    /// If the connection is temporarily unavailable but auto-reconnect is
    /// active, the desired setup is stored and replayed on the next
    /// successful connection.
    Result<EventSubscription> subscribeCaptures(
        const CaptureSubscriptionConfig& config = CaptureSubscriptionConfig(),
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Capture
    // =========================================================================

    /// Trigger a snapshot and wait for all resulting images.
    /// For single-exposure, the vector has one element.
    /// For multi-exposure, each element corresponds to one exposure step.
    /// On timeout, returns partial results (check frames.size() vs
    /// frames[0].info.multiExpLength to detect incomplete groups).
    Result<std::vector<CaptureResult>> captureSnapshot(
        const SnapshotRequest& request = SnapshotRequest(),
        uint32_t timeoutMs = 15000);

    Future<std::vector<CaptureResult>> captureSnapshotAsync(
        const SnapshotRequest& request = SnapshotRequest(),
        uint32_t timeoutMs = 15000);

    /// Get the most recent frame (no new capture triggered).
    Result<std::vector<uint8_t>> getLastFrame(int quality = 80,
                                              uint32_t timeoutMs = 10000);

    Future<std::vector<uint8_t>> getLastFrameAsync(int quality = 80,
                                                   uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Profile management  (sync only)
    // =========================================================================

    /// Return the numeric ID of the currently active profile.
    Result<uint32_t> getActiveProfileId(uint32_t timeoutMs = 10000);

    /// Switch the active profile.
    Result<void> setActiveProfile(uint32_t profileId,
                                  uint32_t timeoutMs = 10000);

    /// List all available profiles (id, name, description, active flag).
    Result<std::vector<ProfileInfo>> listProfiles(uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Trigger & Exposure  (sync only, profile-aware)
    // =========================================================================

    /// @param profileId  Profile to query. Defaults to CURRENT_PROFILE.
    Result<TriggerConfig>  getTriggerConfig(
        uint32_t profileId = CURRENT_PROFILE,
        uint32_t timeoutMs = 10000);

    Result<void>           setTriggerConfig(
        const TriggerConfig& config,
        uint32_t profileId = CURRENT_PROFILE,
        uint32_t timeoutMs = 10000);

    /// @param profileId  Profile to query. Defaults to CURRENT_PROFILE.
    Result<ExposureConfig> getExposureConfig(
        uint32_t profileId = CURRENT_PROFILE,
        uint32_t timeoutMs = 10000);

    Result<void>           setExposureConfig(
        const ExposureConfig& config,
        uint32_t profileId = CURRENT_PROFILE,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Multi-exposure  (sync only, profile-aware)
    // =========================================================================

    /// Read the current multi-exposure configuration for the given profile.
    Result<MultiExposureConfig> getMultiExposureConfig(
        uint32_t profileId = CURRENT_PROFILE,
        uint32_t timeoutMs = 10000);

    /// Write a new multi-exposure configuration for the given profile.
    Result<void> setMultiExposureConfig(
        const MultiExposureConfig& config,
        uint32_t profileId = CURRENT_PROFILE,
        uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Serial communication  (sync only)
    // =========================================================================

    Result<SerialConfig> configureSerial(SerialPort port,
                                         const SerialConfig& config,
                                         uint32_t timeoutMs = 10000);

    Result<int> sendSerialAscii(SerialPort port,
                                const std::string& data,
                                uint32_t timeoutMs = 10000);

    Result<int> sendSerialHex(SerialPort port,
                              const std::string& hexData,
                              uint32_t timeoutMs = 10000);

    Result<int> sendSerialBase64(SerialPort port,
                                 const std::string& b64Data,
                                 uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Typed equipment configuration  (sync only)
    // =========================================================================

    Result<DeviceInfo> getDeviceInfo(uint32_t timeoutMs = 10000);

    Result<void> setScenarioOverlay(int scenario,
                                    const std::string& overlayText,
                                    uint32_t timeoutMs = 10000);

    Result<void> setScenarioCrop(int scenario,
                                 const ScenarioCrop& crop,
                                 uint32_t timeoutMs = 10000);

    Result<void> setSnapshotCrop(bool enable,
                                 const ScenarioCrop& crop,
                                 uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Generic equipment configuration
    // =========================================================================

    /// Return the entire configuration tree as nested JSON.
    Result<nlohmann::json> getAllConfigs(uint32_t timeoutMs = 10000);

    Result<nlohmann::json> getConfig(const std::string& path = "",
                                     uint32_t timeoutMs = 10000);

    Result<nlohmann::json> setConfig(const std::string& path,
                                     const nlohmann::json& data,
                                     uint32_t timeoutMs = 10000);

    // =========================================================================
    //  System
    // =========================================================================

    Result<void>   reboot(uint32_t timeoutMs = 10000);

    // =========================================================================
    //  Event callbacks
    // =========================================================================

    void onTriggerMetadata(std::function<void(const FrameInfo&)> cb);
    void onSnapshotMetadata(std::function<void(const FrameInfo&)> cb);
    void onPreviewMetadata(std::function<void(const FrameInfo&)> cb);

    /// Per-frame image callbacks (fire once for each individual exposure).
    void onTriggerImage(std::function<void(const CaptureResult&)> cb);

    /// Per-frame callback. Fires for externally-triggered snapshots too.
    /// For self-triggered snapshots, prefer captureSnapshot() which returns
    /// the grouped result directly.
    void onSnapshotImage(std::function<void(const CaptureResult&)> cb);
    void onPreviewImage(std::function<void(const CaptureResult&)> cb);

    /// Exposure group callbacks -- fire once per complete multi-exposure
    /// group (all frames sharing the same RID).  For single-exposure
    /// captures, the vector has one element.
    /// Fires after an internal timeout if group is incomplete (partial data).
    void onTriggerExposureGroup(
        std::function<void(const std::vector<CaptureResult>&)> cb);
    void onSnapshotExposureGroup(
        std::function<void(const std::vector<CaptureResult>&)> cb);

    void onPipelineStart(std::function<void(const FrameInfo&)> cb);
    void onGpio(std::function<void(const GpioEvent&)> cb);
    void onSerial(std::function<void(const SerialData&)> cb);
    void onDisconnect(std::function<void(const std::string&)> cb);

    /// Unified connection lifecycle callback.  Fires on:
    ///   Connected    -- initial connection established
    ///   Disconnected -- connection lost (or max retries exceeded)
    ///   Reconnecting -- about to attempt a reconnect
    ///   Reconnected  -- reconnect + session restore succeeded
    void onConnectionStateChanged(
        std::function<void(ConnectionState, const std::string&)> cb);

    // =========================================================================
    //  Advanced
    // =========================================================================

    void setPingInterval(uint32_t seconds);

    /// Max consecutive unanswered pings before declaring the connection dead
    /// and triggering a reconnect.  Default: 10.  Set to 0 to disable.
    void setMaxPingFailures(uint32_t count);

    void setLogHandler(std::function<void(LogLevel, const std::string&)> cb);
    void setDefaultTimeout(uint32_t milliseconds);

    /// Max time (ms) to wait for a multi-exposure group to complete before
    /// delivering partial results.  Default: 5000ms.
    void setExposureGroupTimeout(uint32_t milliseconds);

    // =========================================================================
    //  Low-level session setup and raw protocol access
    // =========================================================================

    /// Lower-level event subscription primitive.
    /// Prefer subscribeCaptures() for the common image-capture workflow.
    /// If the connection is temporarily unavailable but auto-reconnect is
    /// active, the desired subscription is stored and replayed on the next
    /// successful connection.
    Result<EventSubscription> subscribe(const EventSubscription& events,
                                        uint32_t timeoutMs = 10000);

    /// Lower-level JPEG / IMGPKG configuration primitive.
    /// Prefer subscribeCaptures() or JpegConfig::imgpkgDefaults() unless you
    /// need direct control over the protocol-level JPEG settings.
    /// If the connection is temporarily unavailable but auto-reconnect is
    /// active, the desired JPEG config is stored and replayed on the next
    /// successful connection.
    Result<JpegConfig> setJpegConfig(const JpegConfig& config,
                                     uint32_t timeoutMs = 10000);

    /// Lowest-level raw protocol access. Prefer typed SDK methods when
    /// available; this bypasses the higher-level API contracts.
    Result<nlohmann::json> rawCall(uint16_t opcode,
                                   const nlohmann::json& params,
                                   uint32_t timeoutMs = 10000);

    /// Async variant of rawCall().
    Future<nlohmann::json> rawCallAsync(uint16_t opcode,
                                        const nlohmann::json& params,
                                        uint32_t timeoutMs = 10000);

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace itscam

// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix
//
// AUTO-GENERATED FILE -- DO NOT EDIT.
// Regenerate with `make codegen` (see tools/codegen/README.md).
//
// Generated from an OpenAPI 3.0 snapshot of the ITSCAM camera webapp.
// Edit tools/codegen/codegen.mjs and rerun, do not patch this output.
//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     ProfileConfig data = nlohmann::json::parse(jsonString);
//     OcrConfig data = nlohmann::json::parse(jsonString);
//     AnalyticsConfig data = nlohmann::json::parse(jsonString);
//     ClassifierConfig data = nlohmann::json::parse(jsonString);
//     AutoFocus data = nlohmann::json::parse(jsonString);
//     StreamConfig data = nlohmann::json::parse(jsonString);
//     Misc data = nlohmann::json::parse(jsonString);
//     MiscVolatile data = nlohmann::json::parse(jsonString);
//     ItscamproConfig data = nlohmann::json::parse(jsonString);
//     ItscamproStatus data = nlohmann::json::parse(jsonString);
//     ImageSignConfig data = nlohmann::json::parse(jsonString);
//     FtpConfig data = nlohmann::json::parse(jsonString);
//     LinceConfig data = nlohmann::json::parse(jsonString);
//     LinceStatus data = nlohmann::json::parse(jsonString);
//     VehicleIndicatorConfig data = nlohmann::json::parse(jsonString);
//     ProtocolsConfig data = nlohmann::json::parse(jsonString);
//     ProfileTransitioner data = nlohmann::json::parse(jsonString);
//     LanesConfig data = nlohmann::json::parse(jsonString);
//     IoConfig data = nlohmann::json::parse(jsonString);
//     IoBasic data = nlohmann::json::parse(jsonString);
//     RestApiClientConfig data = nlohmann::json::parse(jsonString);
//     RestApiClientStatus data = nlohmann::json::parse(jsonString);
//     Licenses data = nlohmann::json::parse(jsonString);

#pragma once

#include <optional>
#include <nlohmann/json.hpp>

#ifndef NLOHMANN_OPT_HELPER
#define NLOHMANN_OPT_HELPER
namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::shared_ptr<T>> {
        static void to_json(json & j, std::shared_ptr<T> const & opt) {
            if (!opt) j = nullptr; else j = *opt;
        }

        static std::shared_ptr<T> from_json(json const & j) {
            if (j.is_null()) return std::make_shared<T>(); else return std::make_shared<T>(j.get<T>());
        }
    };
    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json & j, std::optional<T> const & opt) {
            if (!opt) j = nullptr; else j = *opt;
        }

        static std::optional<T> from_json(json const & j) {
            if (j.is_null()) return std::make_optional<T>(); else return std::make_optional<T>(j.get<T>());
        }
    };
}
#endif

namespace pumatronix {
namespace itscam {
    using nlohmann::json;

    #ifndef NLOHMANN_UNTYPED_pumatronix_itscam_HELPER
    #define NLOHMANN_UNTYPED_pumatronix_itscam_HELPER
    inline json get_untyped(json const & j, char const * property) {
        if (j.find(property) != j.end()) {
            return j.at(property).get<json>();
        }
        return json();
    }

    inline json get_untyped(json const & j, std::string property) {
        return get_untyped(j, property.data());
    }
    #endif

    #ifndef NLOHMANN_OPTIONAL_pumatronix_itscam_HELPER
    #define NLOHMANN_OPTIONAL_pumatronix_itscam_HELPER
    template <typename T>
    inline std::shared_ptr<T> get_heap_optional(json const & j, char const * property) {
        auto it = j.find(property);
        if (it != j.end() && !it->is_null()) {
            return j.at(property).get<std::shared_ptr<T>>();
        }
        return std::shared_ptr<T>();
    }

    template <typename T>
    inline std::shared_ptr<T> get_heap_optional(json const & j, std::string property) {
        return get_heap_optional<T>(j, property.data());
    }
    template <typename T>
    inline std::optional<T> get_stack_optional(json const & j, char const * property) {
        auto it = j.find(property);
        if (it != j.end() && !it->is_null()) {
            return j.at(property).get<std::optional<T>>();
        }
        return std::nullopt;
    }

    template <typename T>
    inline std::optional<T> get_stack_optional(json const & j, std::string property) {
        return get_stack_optional<T>(j, property.data());
    }
    #endif

    struct Exposition {
        std::optional<int64_t> preferred_shutter;
        std::optional<double> update_factor;
        std::optional<int64_t> update_rate;
    };

    struct AdvancedIris {
        std::optional<int64_t> update_rate;
    };

    struct AdvancedWhitebalance {
        std::optional<int64_t> update_rate;
    };

    struct Advanced {
        std::optional<Exposition> exposition;
        std::optional<AdvancedIris> iris;
        std::optional<AdvancedWhitebalance> whitebalance;
    };

    /**
     * Single RGB value in float format
     */
    struct WhitebalanceClass {
        std::optional<double> blue;
        std::optional<double> green;
        std::optional<double> red;
    };

    /**
     * Camera color configuration fields
     */
    struct Color {
        std::optional<int64_t> blacklevel;
        std::optional<int64_t> brightness;
        std::optional<int64_t> contrast;
        std::optional<WhitebalanceClass> gain;
        std::optional<int64_t> gamma;
        std::optional<int64_t> saturation;
    };

    /**
     * Shutter attributes
     */
    struct ShutterClass {
        std::optional<bool> automatic;
        std::optional<int64_t> fixed_value;
        std::optional<int64_t> max_value;
        std::optional<int64_t> min_value;
    };

    struct ExposureIris {
        std::optional<bool> automatic;
        std::optional<int64_t> fixed_value;
    };

    enum class Mode : int { DISABLED, FAST, NORMAL, SLOW };

    struct Roi1Class {
        std::optional<bool> enabled;
        std::optional<int64_t> x0;
        std::optional<int64_t> x1;
        std::optional<int64_t> x2;
        std::optional<int64_t> x3;
        std::optional<int64_t> y0;
        std::optional<int64_t> y1;
        std::optional<int64_t> y2;
        std::optional<int64_t> y3;
    };

    struct Level {
        std::optional<int64_t> hold_time;
        std::optional<Mode> mode;
        std::optional<Roi1Class> roi;
        std::optional<double> target_value;
        std::optional<int64_t> update_rate;
    };

    /**
     * Camera exposure configuration fields
     */
    struct Exposure {
        std::optional<ShutterClass> gain;
        std::optional<ExposureIris> iris;
        std::optional<Level> level;
        std::optional<ShutterClass> shutter;
    };

    struct Hdr {
        std::optional<bool> enable;
    };

    /**
     * Camera lens configuration fields
     */
    struct ProfileConfigLens {
        std::optional<bool> exchanger;
        std::optional<int64_t> focus;
        std::optional<bool> zf_mirror_profile0;
        std::optional<int64_t> zoom;
    };

    struct MovFilter {
        std::optional<bool> enabled;
        std::optional<bool> only_check;
        std::optional<Roi1Class> roi;
        std::optional<double> threshold;
    };

    struct Power {
        std::optional<int64_t> out;
        std::optional<int64_t> percent;
    };

    /**
     * Camera flash configuration
     */
    struct Flash {
        std::optional<std::vector<Power>> power;
    };

    struct SettingGain {
        std::optional<bool> percentage_of_current;
        std::optional<double> value;
    };

    struct Shutter {
        std::optional<bool> percentage_of_current;
        std::optional<double> value;
    };

    /**
     * Multiple exposures configuration
     */
    struct Something {
        std::optional<Flash> flash;
        std::optional<SettingGain> gain;
        std::optional<Shutter> shutter;
    };

    struct MultipleExposures {
        std::optional<bool> enabled;
        std::optional<std::vector<Something>> settings;
    };

    struct Overlay {
        std::optional<bool> enable;
        std::optional<std::string> text;
    };

    struct Lower {
        std::optional<std::string> end_time;
        std::optional<int64_t> hold_time;
        std::optional<double> level;
        std::optional<int64_t> profile;
        std::optional<std::string> start_time;
    };

    /**
     * Camera profile transition configuration
     */
    struct Transitions {
        std::optional<Lower> lower;
        std::optional<Lower> upper;
    };

    struct Trigger {
        std::optional<bool> enabled;
        std::optional<std::string> event;
        std::optional<int64_t> minimum_interval;
        std::optional<int64_t> port;
        std::optional<Roi1Class> roi;
        std::optional<double> threshold;
    };

    /**
     * Camera white balance configuration fields
     */
    struct ProfileConfigWhitebalance {
        std::optional<bool> automatic;
        std::optional<WhitebalanceClass> weights;
    };

    /**
     * Camera profile configuration
     */
    struct ProfileConfig {
        std::optional<bool> active;
        std::optional<Advanced> advanced;
        std::optional<Color> color;
        std::optional<std::string> description;
        std::optional<Exposure> exposure;
        std::optional<Hdr> hdr;
        int64_t id;
        std::optional<ProfileConfigLens> lens;
        std::optional<MovFilter> mov_filter;
        std::optional<MultipleExposures> multiple_exposures;
        std::optional<std::string> name;
        std::optional<Overlay> overlay;
        std::optional<Transitions> transitions;
        std::optional<Trigger> trigger;
        std::optional<ProfileConfigWhitebalance> whitebalance;
    };

    struct OcrConfigOcr {
        std::optional<int64_t> avg_char_height;
        std::optional<double> avg_plate_angle;
        std::optional<double> avg_plate_slant;
        std::optional<double> classifier_expansion;
        std::optional<int64_t> country_code;
        std::optional<bool> enabled;
        std::optional<bool> licensed;
        std::optional<int64_t> max_char_height;
        std::optional<int64_t> max_low_prob_chars;
        std::optional<int64_t> max_plates;
        std::optional<int64_t> min_char_height;
        std::optional<double> min_prob_per_char;
        std::optional<int64_t> processing_mode;
        std::optional<int64_t> processing_queue;
        std::optional<int64_t> processing_threads;
        std::optional<int64_t> processing_timeout;
        std::optional<Roi1Class> roi;
        std::optional<bool> use_classifier_result;
        std::optional<int64_t> vehicle_type;
    };

    /**
     * Ocr service configuration
     */
    struct OcrConfig {
        std::optional<OcrConfigOcr> ocr;
    };

    struct Voting {
        std::optional<bool> enabled;
        std::optional<bool> forward_without_plate_if_tracker;
        std::optional<bool> keep_best_only;
        std::optional<int64_t> max_diff_chars;
        std::optional<Roi1Class> roi1;
        std::optional<Roi1Class> roi2;
        std::optional<int64_t> same_plate_debounce;
        std::optional<bool> use_classifier;
    };

    /**
     * Plate Analytics service configuration
     */
    struct AnalyticsConfig {
        std::optional<Voting> voting;
    };

    /**
     * Classifier based speed calibration region;
     */
    struct SpeedCalibrationRegion1 {
        std::optional<double> p0_top1_sz;
        std::optional<int64_t> x0;
        std::optional<int64_t> x1;
        std::optional<int64_t> x2;
        std::optional<int64_t> y0;
        std::optional<int64_t> y1;
        std::optional<int64_t> y2;
    };

    /**
     * Classifier based trigger region; * dir: top-bottom, bottom-top, disabled
     */
    struct TriggerRegion0 {
        std::optional<std::string> dir;
        std::optional<int64_t> x0;
        std::optional<int64_t> x1;
        std::optional<int64_t> y0;
        std::optional<int64_t> y1;
    };

    struct ClassifierConfigClassifier {
        std::optional<bool> enable_characteristics;
        std::optional<bool> enabled;
        std::optional<bool> enable_speed;
        std::optional<bool> first_only;
        std::optional<bool> licensed;
        std::optional<double> min_probability;
        std::optional<int64_t> model_type;
        std::optional<int64_t> processing_queue;
        std::optional<int64_t> processing_threads;
        std::optional<int64_t> scene_type;
        std::optional<SpeedCalibrationRegion1> speed_calibration_region1;
        std::optional<SpeedCalibrationRegion1> speed_calibration_region2;
        std::optional<bool> trigger_enabled;
        std::optional<TriggerRegion0> trigger_region0;
        std::optional<TriggerRegion0> trigger_region1;
        std::optional<TriggerRegion0> trigger_region2;
        std::optional<TriggerRegion0> trigger_region3;
    };

    /**
     * Vehicle Classifier service configuration
     */
    struct ClassifierConfig {
        std::optional<ClassifierConfigClassifier> classifier;
    };

    struct AutoFocusRoi {
        std::optional<int64_t> center_x;
        std::optional<int64_t> center_y;
        std::optional<int64_t> height;
        std::optional<int64_t> width;
    };

    /**
     * AutoFocus configs
     */
    struct AutoFocus {
        std::optional<int64_t> coarse_step;
        std::optional<double> contrast_threshold;
        std::optional<AutoFocusRoi> roi;
        std::optional<bool> run;
        std::optional<int64_t> update_rate;
    };

    /**
     * Single h26x stream configuration
     */
    struct H264Main {
        std::optional<bool> available;
        std::optional<int64_t> bitrate;
        std::optional<std::string> control_rate;
        std::optional<bool> enabled;
        std::optional<int64_t> gop;
        std::optional<std::string> profile;
        std::optional<bool> running;
        std::optional<std::string> source;
    };

    struct H264 {
        std::optional<bool> available;
        std::optional<std::string> encoder_type;
        std::optional<H264Main> main;
    };

    /**
     * Single MJPEG stream configuration
     */
    struct MjpegMain {
        std::optional<bool> available;
        std::optional<bool> enabled;
        std::optional<double> framerate;
        std::optional<int64_t> quality;
    };

    struct Mjpeg {
        std::optional<bool> available;
        std::optional<MjpegMain> main;
    };

    /**
     * Stream(s) configuration of device
     */
    struct StreamConfig {
        std::optional<H264> h264;
        std::optional<Mjpeg> mjpeg;
    };

    struct Scenario1Crop {
        std::optional<int64_t> x0;
        std::optional<int64_t> x1;
        std::optional<int64_t> y0;
        std::optional<int64_t> y1;
    };

    struct Scenario2Crop {
        std::optional<int64_t> x0;
        std::optional<int64_t> x1;
        std::optional<int64_t> y0;
        std::optional<int64_t> y1;
    };

    struct SnapshotCrop {
        std::optional<bool> enable;
        std::optional<std::string> mode;
        std::optional<int64_t> x0;
        std::optional<int64_t> x1;
        std::optional<int64_t> y0;
        std::optional<int64_t> y1;
    };

    /**
     * Miscellaneous configs
     */
    struct Misc {
        std::optional<bool> camera_orientation;
        std::optional<std::string> iris_hint;
        std::optional<int64_t> jpeg_quality;
        std::optional<int64_t> legacy_tsync_gpio;
        std::optional<Scenario1Crop> scenario1_crop;
        std::optional<std::string> scenario1_overlay;
        std::optional<int64_t> scenario1_overlay_text_size;
        std::optional<Scenario2Crop> scenario2_crop;
        std::optional<std::string> scenario2_overlay;
        std::optional<int64_t> scenario2_overlay_text_size;
        std::optional<std::string> scenario_overlay_color;
        std::optional<SnapshotCrop> snapshot_crop;
    };

    struct Ae {
        std::optional<std::string> ctrl_mode;
        std::optional<int64_t> last_run;
        std::optional<double> level;
    };

    struct Fps {
        std::optional<double> mjpeg;
    };

    struct Gps {
        std::optional<double> altitude;
        std::optional<bool> available;
        std::optional<double> bearing;
        std::optional<double> dop;
        std::optional<std::string> fix;
        std::optional<double> latitude;
        std::optional<double> longitude;
        std::optional<int64_t> num_satellites;
        std::optional<int64_t> seconds_since_last_fix;
        std::optional<double> speed;
        std::optional<int64_t> time;
    };

    struct Isp {
        std::optional<int64_t> free_buffers;
        std::optional<int64_t> gain;
        std::optional<int64_t> iris;
        std::optional<std::string> iris_model;
        std::optional<int64_t> shutter;
    };

    struct MiscVolatileLens {
        std::optional<int64_t> focus;
        std::optional<int64_t> zoom;
    };

    struct Profile {
        std::optional<int64_t> id;
        std::optional<std::string> name;
    };

    /**
     * Current Miscellaneous Read-Only configs
     */
    struct MiscVolatile {
        std::optional<Ae> ae;
        std::optional<Fps> fps;
        std::optional<Gps> gps;
        std::optional<Isp> isp;
        std::optional<MiscVolatileLens> lens;
        std::optional<Profile> profile;
        std::optional<WhitebalanceClass> whitebalance;
    };

    struct Itscampro {
        std::optional<std::string> address;
        std::optional<bool> debug;
        std::optional<bool> enable;
        std::optional<int64_t> port;
    };

    /**
     * ITSCAMPRO service configuration
     */
    struct ItscamproConfig {
        std::optional<Itscampro> itscampro;
    };

    /**
     * ITSCAMPRO service status
     */
    struct ItscamproStatus {
        std::optional<std::string> status;
    };

    struct Sign {
        std::optional<std::string> append_mode;
        std::optional<bool> enabled;
        std::optional<bool> loaded;
        std::optional<bool> update;
    };

    /**
     * ImageSign service configuration
     */
    struct ImageSignConfig {
        std::optional<Sign> sign;
    };

    struct Local {
        std::optional<int64_t> buffer_size_kb;
        std::optional<int64_t> ttl;
    };

    struct Transfer {
        std::optional<int64_t> poll_interval;
        std::optional<int64_t> timeout;
    };

    struct Ftp {
        std::optional<std::string> address;
        std::optional<bool> anonymous;
        std::optional<bool> enable;
        std::optional<std::string> filename;
        std::optional<Local> local;
        std::optional<std::string> password;
        std::optional<int64_t> port;
        std::optional<std::string> protocol;
        std::optional<int64_t> quality;
        std::optional<Transfer> transfer;
        std::optional<std::string> username;
    };

    /**
     * FTP service configuration
     */
    struct FtpConfig {
        std::optional<Ftp> ftp;
    };

    /**
     * Lince service configuration
     */
    struct LinceConfig {
        std::optional<std::string> auth_code;
        std::optional<std::string> client_endpoint;
        std::optional<std::string> client_id;
        std::optional<bool> enabled;
        std::optional<std::string> environment;
        std::optional<bool> send_recs_none;
        std::optional<int64_t> timeout_response;
    };

    /**
     * Lince service status
     */
    struct LinceStatus {
        std::optional<std::string> lince_status;
    };

    struct VehicleIndicator {
        std::optional<bool> vehicle_counter_active_high;
        std::optional<bool> vehicle_counter_enabled;
        std::optional<int64_t> vehicle_counter_gpio;
        std::optional<int64_t> vehicle_counter_pulse_width_ms;
        std::optional<int64_t> vehicle_counter_type;
        std::optional<int64_t> vehicle_counter_udp_port;
        std::optional<int64_t> vehicle_counter_udp_sample_time_ms;
        std::optional<std::string> vehicle_counter_udp_server;
    };

    /**
     * VehicleIndicator service configuration
     */
    struct VehicleIndicatorConfig {
        std::optional<VehicleIndicator> vehicle_indicator;
    };

    struct ConfigCgi {
        std::optional<bool> block_api;
    };

    struct Auth {
        std::optional<std::string> password;
        std::optional<bool> require;
    };

    struct Cougar {
        std::optional<Auth> auth;
    };

    struct Itscamprotocol {
        std::optional<bool> legacy_mode;
    };

    /**
     * Protocols configurations
     */
    struct ProtocolsConfig {
        std::optional<ConfigCgi> config_cgi;
        std::optional<Cougar> cougar;
        std::optional<Itscamprotocol> itscamprotocol;
    };

    /**
     * Profile Transitioner configs
     */
    struct ProfileTransitioner {
        std::optional<bool> automatic;
        std::optional<std::string> level_smoothing;
        std::optional<std::string> reset_profiles;
        std::optional<int64_t> smoothing_time;
    };

    /**
     * Lane region
     */
    struct Region0 {
        std::optional<std::string> name;
        std::optional<int64_t> x0;
        std::optional<int64_t> x1;
        std::optional<int64_t> x2;
        std::optional<int64_t> x3;
        std::optional<int64_t> y0;
        std::optional<int64_t> y1;
        std::optional<int64_t> y2;
        std::optional<int64_t> y3;
    };

    /**
     * Lanes configuration
     */
    struct LanesConfig {
        std::optional<bool> enabled;
        std::optional<Region0> region0;
        std::optional<Region0> region1;
        std::optional<Region0> region2;
    };

    /**
     * Configuration for a specific IO
     */
    struct IoConfig {
        std::optional<bool> can_flash;
        std::optional<bool> can_trigger;
        std::optional<int64_t> early_us;
        std::optional<std::string> group;
        std::optional<std::string> identifier;
        std::optional<bool> is_input;
        std::optional<bool> is_on;
        int64_t port;
        std::optional<std::string> protection;
        std::optional<std::string> type;
    };

    /**
     * Simplified information for a specific IO
     */
    struct IoBasic {
        std::optional<bool> is_input;
        std::optional<bool> is_on;
        int64_t port;
    };

    enum class Type : int { JSON };

    struct Part {
        std::string content;
        std::string name;
        Type type;
    };

    enum class Variant : int { MULTIPART, SINGLEPART };

    struct Body {
        std::vector<Part> parts;
        Variant variant;
    };

    struct Header {
        std::string name;
        std::string value;
    };

    struct Resolution {
        int64_t height;
        int64_t width;
    };

    struct Jpeg {
        int64_t quality;
        Resolution resolution;
    };

    enum class Method : int { GET, POST, PUT };

    struct Persistency {
        bool enabled;
        int64_t max_disk_usage;
        int64_t max_file_age;
        bool newest_first;
    };

    enum class Scheme : int { HTTP, HTTPS };

    struct Url {
        std::string host;
        std::string path;
        std::vector<std::string> query;
        Scheme scheme;
    };

    /**
     * REST API Client service configuration
     */
    struct RestApiClientConfig {
        Body body;
        bool enabled;
        std::vector<Header> headers;
        Jpeg jpeg;
        Method method;
        Persistency persistency;
        int64_t retries;
        bool send_individual_requests;
        bool send_without_ocr;
        int64_t timeout;
        Url url;
    };

    /**
     * REST API Client service status
     */
    struct RestApiClientStatus {
        int64_t code;
        int64_t disk_usage;
        int64_t file_count;
        std::string message;
    };

    struct AnalyticsClassifier {
        std::optional<std::string> customer;
        std::optional<int64_t> max_connections;
        std::optional<int64_t> max_threads;
        std::optional<std::string> serial;
        std::optional<std::string> sha1;
        std::optional<int64_t> state;
        std::optional<int64_t> ttl;
        std::optional<std::string> version;
    };

    struct AnalyticsOcr {
        std::optional<std::string> customer;
        std::optional<int64_t> max_connections;
        std::optional<int64_t> max_threads;
        std::optional<std::string> serial;
        std::optional<std::string> sha1;
        std::optional<int64_t> state;
        std::optional<int64_t> ttl;
        std::optional<std::string> version;
    };

    struct Analytics {
        std::optional<AnalyticsClassifier> classifier;
        std::optional<AnalyticsOcr> ocr;
    };

    struct DeviceId {
        std::optional<std::string> serial;
    };

    /**
     * License service information
     */
    struct Licenses {
        std::optional<Analytics> analytics;
        std::optional<DeviceId> device_id;
    };
}
}

namespace pumatronix {
namespace itscam {
    void from_json(json const & j, Exposition & x);
    void to_json(json & j, Exposition const & x);

    void from_json(json const & j, AdvancedIris & x);
    void to_json(json & j, AdvancedIris const & x);

    void from_json(json const & j, AdvancedWhitebalance & x);
    void to_json(json & j, AdvancedWhitebalance const & x);

    void from_json(json const & j, Advanced & x);
    void to_json(json & j, Advanced const & x);

    void from_json(json const & j, WhitebalanceClass & x);
    void to_json(json & j, WhitebalanceClass const & x);

    void from_json(json const & j, Color & x);
    void to_json(json & j, Color const & x);

    void from_json(json const & j, ShutterClass & x);
    void to_json(json & j, ShutterClass const & x);

    void from_json(json const & j, ExposureIris & x);
    void to_json(json & j, ExposureIris const & x);

    void from_json(json const & j, Roi1Class & x);
    void to_json(json & j, Roi1Class const & x);

    void from_json(json const & j, Level & x);
    void to_json(json & j, Level const & x);

    void from_json(json const & j, Exposure & x);
    void to_json(json & j, Exposure const & x);

    void from_json(json const & j, Hdr & x);
    void to_json(json & j, Hdr const & x);

    void from_json(json const & j, ProfileConfigLens & x);
    void to_json(json & j, ProfileConfigLens const & x);

    void from_json(json const & j, MovFilter & x);
    void to_json(json & j, MovFilter const & x);

    void from_json(json const & j, Power & x);
    void to_json(json & j, Power const & x);

    void from_json(json const & j, Flash & x);
    void to_json(json & j, Flash const & x);

    void from_json(json const & j, SettingGain & x);
    void to_json(json & j, SettingGain const & x);

    void from_json(json const & j, Shutter & x);
    void to_json(json & j, Shutter const & x);

    void from_json(json const & j, Something & x);
    void to_json(json & j, Something const & x);

    void from_json(json const & j, MultipleExposures & x);
    void to_json(json & j, MultipleExposures const & x);

    void from_json(json const & j, Overlay & x);
    void to_json(json & j, Overlay const & x);

    void from_json(json const & j, Lower & x);
    void to_json(json & j, Lower const & x);

    void from_json(json const & j, Transitions & x);
    void to_json(json & j, Transitions const & x);

    void from_json(json const & j, Trigger & x);
    void to_json(json & j, Trigger const & x);

    void from_json(json const & j, ProfileConfigWhitebalance & x);
    void to_json(json & j, ProfileConfigWhitebalance const & x);

    void from_json(json const & j, ProfileConfig & x);
    void to_json(json & j, ProfileConfig const & x);

    void from_json(json const & j, OcrConfigOcr & x);
    void to_json(json & j, OcrConfigOcr const & x);

    void from_json(json const & j, OcrConfig & x);
    void to_json(json & j, OcrConfig const & x);

    void from_json(json const & j, Voting & x);
    void to_json(json & j, Voting const & x);

    void from_json(json const & j, AnalyticsConfig & x);
    void to_json(json & j, AnalyticsConfig const & x);

    void from_json(json const & j, SpeedCalibrationRegion1 & x);
    void to_json(json & j, SpeedCalibrationRegion1 const & x);

    void from_json(json const & j, TriggerRegion0 & x);
    void to_json(json & j, TriggerRegion0 const & x);

    void from_json(json const & j, ClassifierConfigClassifier & x);
    void to_json(json & j, ClassifierConfigClassifier const & x);

    void from_json(json const & j, ClassifierConfig & x);
    void to_json(json & j, ClassifierConfig const & x);

    void from_json(json const & j, AutoFocusRoi & x);
    void to_json(json & j, AutoFocusRoi const & x);

    void from_json(json const & j, AutoFocus & x);
    void to_json(json & j, AutoFocus const & x);

    void from_json(json const & j, H264Main & x);
    void to_json(json & j, H264Main const & x);

    void from_json(json const & j, H264 & x);
    void to_json(json & j, H264 const & x);

    void from_json(json const & j, MjpegMain & x);
    void to_json(json & j, MjpegMain const & x);

    void from_json(json const & j, Mjpeg & x);
    void to_json(json & j, Mjpeg const & x);

    void from_json(json const & j, StreamConfig & x);
    void to_json(json & j, StreamConfig const & x);

    void from_json(json const & j, Scenario1Crop & x);
    void to_json(json & j, Scenario1Crop const & x);

    void from_json(json const & j, Scenario2Crop & x);
    void to_json(json & j, Scenario2Crop const & x);

    void from_json(json const & j, SnapshotCrop & x);
    void to_json(json & j, SnapshotCrop const & x);

    void from_json(json const & j, Misc & x);
    void to_json(json & j, Misc const & x);

    void from_json(json const & j, Ae & x);
    void to_json(json & j, Ae const & x);

    void from_json(json const & j, Fps & x);
    void to_json(json & j, Fps const & x);

    void from_json(json const & j, Gps & x);
    void to_json(json & j, Gps const & x);

    void from_json(json const & j, Isp & x);
    void to_json(json & j, Isp const & x);

    void from_json(json const & j, MiscVolatileLens & x);
    void to_json(json & j, MiscVolatileLens const & x);

    void from_json(json const & j, Profile & x);
    void to_json(json & j, Profile const & x);

    void from_json(json const & j, MiscVolatile & x);
    void to_json(json & j, MiscVolatile const & x);

    void from_json(json const & j, Itscampro & x);
    void to_json(json & j, Itscampro const & x);

    void from_json(json const & j, ItscamproConfig & x);
    void to_json(json & j, ItscamproConfig const & x);

    void from_json(json const & j, ItscamproStatus & x);
    void to_json(json & j, ItscamproStatus const & x);

    void from_json(json const & j, Sign & x);
    void to_json(json & j, Sign const & x);

    void from_json(json const & j, ImageSignConfig & x);
    void to_json(json & j, ImageSignConfig const & x);

    void from_json(json const & j, Local & x);
    void to_json(json & j, Local const & x);

    void from_json(json const & j, Transfer & x);
    void to_json(json & j, Transfer const & x);

    void from_json(json const & j, Ftp & x);
    void to_json(json & j, Ftp const & x);

    void from_json(json const & j, FtpConfig & x);
    void to_json(json & j, FtpConfig const & x);

    void from_json(json const & j, LinceConfig & x);
    void to_json(json & j, LinceConfig const & x);

    void from_json(json const & j, LinceStatus & x);
    void to_json(json & j, LinceStatus const & x);

    void from_json(json const & j, VehicleIndicator & x);
    void to_json(json & j, VehicleIndicator const & x);

    void from_json(json const & j, VehicleIndicatorConfig & x);
    void to_json(json & j, VehicleIndicatorConfig const & x);

    void from_json(json const & j, ConfigCgi & x);
    void to_json(json & j, ConfigCgi const & x);

    void from_json(json const & j, Auth & x);
    void to_json(json & j, Auth const & x);

    void from_json(json const & j, Cougar & x);
    void to_json(json & j, Cougar const & x);

    void from_json(json const & j, Itscamprotocol & x);
    void to_json(json & j, Itscamprotocol const & x);

    void from_json(json const & j, ProtocolsConfig & x);
    void to_json(json & j, ProtocolsConfig const & x);

    void from_json(json const & j, ProfileTransitioner & x);
    void to_json(json & j, ProfileTransitioner const & x);

    void from_json(json const & j, Region0 & x);
    void to_json(json & j, Region0 const & x);

    void from_json(json const & j, LanesConfig & x);
    void to_json(json & j, LanesConfig const & x);

    void from_json(json const & j, IoConfig & x);
    void to_json(json & j, IoConfig const & x);

    void from_json(json const & j, IoBasic & x);
    void to_json(json & j, IoBasic const & x);

    void from_json(json const & j, Part & x);
    void to_json(json & j, Part const & x);

    void from_json(json const & j, Body & x);
    void to_json(json & j, Body const & x);

    void from_json(json const & j, Header & x);
    void to_json(json & j, Header const & x);

    void from_json(json const & j, Resolution & x);
    void to_json(json & j, Resolution const & x);

    void from_json(json const & j, Jpeg & x);
    void to_json(json & j, Jpeg const & x);

    void from_json(json const & j, Persistency & x);
    void to_json(json & j, Persistency const & x);

    void from_json(json const & j, Url & x);
    void to_json(json & j, Url const & x);

    void from_json(json const & j, RestApiClientConfig & x);
    void to_json(json & j, RestApiClientConfig const & x);

    void from_json(json const & j, RestApiClientStatus & x);
    void to_json(json & j, RestApiClientStatus const & x);

    void from_json(json const & j, AnalyticsClassifier & x);
    void to_json(json & j, AnalyticsClassifier const & x);

    void from_json(json const & j, AnalyticsOcr & x);
    void to_json(json & j, AnalyticsOcr const & x);

    void from_json(json const & j, Analytics & x);
    void to_json(json & j, Analytics const & x);

    void from_json(json const & j, DeviceId & x);
    void to_json(json & j, DeviceId const & x);

    void from_json(json const & j, Licenses & x);
    void to_json(json & j, Licenses const & x);

    void from_json(json const & j, Mode & x);
    void to_json(json & j, Mode const & x);

    void from_json(json const & j, Type & x);
    void to_json(json & j, Type const & x);

    void from_json(json const & j, Variant & x);
    void to_json(json & j, Variant const & x);

    void from_json(json const & j, Method & x);
    void to_json(json & j, Method const & x);

    void from_json(json const & j, Scheme & x);
    void to_json(json & j, Scheme const & x);

    // Partial-JSON serializers (omit nullopt fields for typed partial PUT)
    json to_partial_json(Exposition const & x);
    json to_partial_json(AdvancedIris const & x);
    json to_partial_json(AdvancedWhitebalance const & x);
    json to_partial_json(Advanced const & x);
    json to_partial_json(WhitebalanceClass const & x);
    json to_partial_json(Color const & x);
    json to_partial_json(ShutterClass const & x);
    json to_partial_json(ExposureIris const & x);
    json to_partial_json(Roi1Class const & x);
    json to_partial_json(Level const & x);
    json to_partial_json(Exposure const & x);
    json to_partial_json(Hdr const & x);
    json to_partial_json(ProfileConfigLens const & x);
    json to_partial_json(MovFilter const & x);
    json to_partial_json(Power const & x);
    json to_partial_json(Flash const & x);
    json to_partial_json(SettingGain const & x);
    json to_partial_json(Shutter const & x);
    json to_partial_json(Something const & x);
    json to_partial_json(MultipleExposures const & x);
    json to_partial_json(Overlay const & x);
    json to_partial_json(Lower const & x);
    json to_partial_json(Transitions const & x);
    json to_partial_json(Trigger const & x);
    json to_partial_json(ProfileConfigWhitebalance const & x);
    json to_partial_json(ProfileConfig const & x);
    json to_partial_json(OcrConfigOcr const & x);
    json to_partial_json(OcrConfig const & x);
    json to_partial_json(Voting const & x);
    json to_partial_json(AnalyticsConfig const & x);
    json to_partial_json(SpeedCalibrationRegion1 const & x);
    json to_partial_json(TriggerRegion0 const & x);
    json to_partial_json(ClassifierConfigClassifier const & x);
    json to_partial_json(ClassifierConfig const & x);
    json to_partial_json(AutoFocusRoi const & x);
    json to_partial_json(AutoFocus const & x);
    json to_partial_json(H264Main const & x);
    json to_partial_json(H264 const & x);
    json to_partial_json(MjpegMain const & x);
    json to_partial_json(Mjpeg const & x);
    json to_partial_json(StreamConfig const & x);
    json to_partial_json(Scenario1Crop const & x);
    json to_partial_json(Scenario2Crop const & x);
    json to_partial_json(SnapshotCrop const & x);
    json to_partial_json(Misc const & x);
    json to_partial_json(Ae const & x);
    json to_partial_json(Fps const & x);
    json to_partial_json(Gps const & x);
    json to_partial_json(Isp const & x);
    json to_partial_json(MiscVolatileLens const & x);
    json to_partial_json(Profile const & x);
    json to_partial_json(MiscVolatile const & x);
    json to_partial_json(Itscampro const & x);
    json to_partial_json(ItscamproConfig const & x);
    json to_partial_json(ItscamproStatus const & x);
    json to_partial_json(Sign const & x);
    json to_partial_json(ImageSignConfig const & x);
    json to_partial_json(Local const & x);
    json to_partial_json(Transfer const & x);
    json to_partial_json(Ftp const & x);
    json to_partial_json(FtpConfig const & x);
    json to_partial_json(LinceConfig const & x);
    json to_partial_json(LinceStatus const & x);
    json to_partial_json(VehicleIndicator const & x);
    json to_partial_json(VehicleIndicatorConfig const & x);
    json to_partial_json(ConfigCgi const & x);
    json to_partial_json(Auth const & x);
    json to_partial_json(Cougar const & x);
    json to_partial_json(Itscamprotocol const & x);
    json to_partial_json(ProtocolsConfig const & x);
    json to_partial_json(ProfileTransitioner const & x);
    json to_partial_json(Region0 const & x);
    json to_partial_json(LanesConfig const & x);
    json to_partial_json(IoConfig const & x);
    json to_partial_json(IoBasic const & x);
    json to_partial_json(Part const & x);
    json to_partial_json(Body const & x);
    json to_partial_json(Header const & x);
    json to_partial_json(Resolution const & x);
    json to_partial_json(Jpeg const & x);
    json to_partial_json(Persistency const & x);
    json to_partial_json(Url const & x);
    json to_partial_json(RestApiClientConfig const & x);
    json to_partial_json(RestApiClientStatus const & x);
    json to_partial_json(AnalyticsClassifier const & x);
    json to_partial_json(AnalyticsOcr const & x);
    json to_partial_json(Analytics const & x);
    json to_partial_json(DeviceId const & x);
    json to_partial_json(Licenses const & x);

    inline void from_json(json const & j, Exposition& x) {
        x.preferred_shutter = get_stack_optional<int64_t>(j, "preferredShutter");
        x.update_factor = get_stack_optional<double>(j, "updateFactor");
        x.update_rate = get_stack_optional<int64_t>(j, "updateRate");
    }

    inline void to_json(json & j, Exposition const & x) {
        j = json::object();
        j["preferredShutter"] = x.preferred_shutter;
        j["updateFactor"] = x.update_factor;
        j["updateRate"] = x.update_rate;
    }

    inline void from_json(json const & j, AdvancedIris& x) {
        x.update_rate = get_stack_optional<int64_t>(j, "updateRate");
    }

    inline void to_json(json & j, AdvancedIris const & x) {
        j = json::object();
        j["updateRate"] = x.update_rate;
    }

    inline void from_json(json const & j, AdvancedWhitebalance& x) {
        x.update_rate = get_stack_optional<int64_t>(j, "updateRate");
    }

    inline void to_json(json & j, AdvancedWhitebalance const & x) {
        j = json::object();
        j["updateRate"] = x.update_rate;
    }

    inline void from_json(json const & j, Advanced& x) {
        x.exposition = get_stack_optional<Exposition>(j, "exposition");
        x.iris = get_stack_optional<AdvancedIris>(j, "iris");
        x.whitebalance = get_stack_optional<AdvancedWhitebalance>(j, "whitebalance");
    }

    inline void to_json(json & j, Advanced const & x) {
        j = json::object();
        j["exposition"] = x.exposition;
        j["iris"] = x.iris;
        j["whitebalance"] = x.whitebalance;
    }

    inline void from_json(json const & j, WhitebalanceClass& x) {
        x.blue = get_stack_optional<double>(j, "blue");
        x.green = get_stack_optional<double>(j, "green");
        x.red = get_stack_optional<double>(j, "red");
    }

    inline void to_json(json & j, WhitebalanceClass const & x) {
        j = json::object();
        j["blue"] = x.blue;
        j["green"] = x.green;
        j["red"] = x.red;
    }

    inline void from_json(json const & j, Color& x) {
        x.blacklevel = get_stack_optional<int64_t>(j, "blacklevel");
        x.brightness = get_stack_optional<int64_t>(j, "brightness");
        x.contrast = get_stack_optional<int64_t>(j, "contrast");
        x.gain = get_stack_optional<WhitebalanceClass>(j, "gain");
        x.gamma = get_stack_optional<int64_t>(j, "gamma");
        x.saturation = get_stack_optional<int64_t>(j, "saturation");
    }

    inline void to_json(json & j, Color const & x) {
        j = json::object();
        j["blacklevel"] = x.blacklevel;
        j["brightness"] = x.brightness;
        j["contrast"] = x.contrast;
        j["gain"] = x.gain;
        j["gamma"] = x.gamma;
        j["saturation"] = x.saturation;
    }

    inline void from_json(json const & j, ShutterClass& x) {
        x.automatic = get_stack_optional<bool>(j, "automatic");
        x.fixed_value = get_stack_optional<int64_t>(j, "fixedValue");
        x.max_value = get_stack_optional<int64_t>(j, "maxValue");
        x.min_value = get_stack_optional<int64_t>(j, "minValue");
    }

    inline void to_json(json & j, ShutterClass const & x) {
        j = json::object();
        j["automatic"] = x.automatic;
        j["fixedValue"] = x.fixed_value;
        j["maxValue"] = x.max_value;
        j["minValue"] = x.min_value;
    }

    inline void from_json(json const & j, ExposureIris& x) {
        x.automatic = get_stack_optional<bool>(j, "automatic");
        x.fixed_value = get_stack_optional<int64_t>(j, "fixedValue");
    }

    inline void to_json(json & j, ExposureIris const & x) {
        j = json::object();
        j["automatic"] = x.automatic;
        j["fixedValue"] = x.fixed_value;
    }

    inline void from_json(json const & j, Roi1Class& x) {
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.x0 = get_stack_optional<int64_t>(j, "x0");
        x.x1 = get_stack_optional<int64_t>(j, "x1");
        x.x2 = get_stack_optional<int64_t>(j, "x2");
        x.x3 = get_stack_optional<int64_t>(j, "x3");
        x.y0 = get_stack_optional<int64_t>(j, "y0");
        x.y1 = get_stack_optional<int64_t>(j, "y1");
        x.y2 = get_stack_optional<int64_t>(j, "y2");
        x.y3 = get_stack_optional<int64_t>(j, "y3");
    }

    inline void to_json(json & j, Roi1Class const & x) {
        j = json::object();
        j["enabled"] = x.enabled;
        j["x0"] = x.x0;
        j["x1"] = x.x1;
        j["x2"] = x.x2;
        j["x3"] = x.x3;
        j["y0"] = x.y0;
        j["y1"] = x.y1;
        j["y2"] = x.y2;
        j["y3"] = x.y3;
    }

    inline void from_json(json const & j, Level& x) {
        x.hold_time = get_stack_optional<int64_t>(j, "holdTime");
        x.mode = get_stack_optional<Mode>(j, "mode");
        x.roi = get_stack_optional<Roi1Class>(j, "roi");
        x.target_value = get_stack_optional<double>(j, "targetValue");
        x.update_rate = get_stack_optional<int64_t>(j, "updateRate");
    }

    inline void to_json(json & j, Level const & x) {
        j = json::object();
        j["holdTime"] = x.hold_time;
        j["mode"] = x.mode;
        j["roi"] = x.roi;
        j["targetValue"] = x.target_value;
        j["updateRate"] = x.update_rate;
    }

    inline void from_json(json const & j, Exposure& x) {
        x.gain = get_stack_optional<ShutterClass>(j, "gain");
        x.iris = get_stack_optional<ExposureIris>(j, "iris");
        x.level = get_stack_optional<Level>(j, "level");
        x.shutter = get_stack_optional<ShutterClass>(j, "shutter");
    }

    inline void to_json(json & j, Exposure const & x) {
        j = json::object();
        j["gain"] = x.gain;
        j["iris"] = x.iris;
        j["level"] = x.level;
        j["shutter"] = x.shutter;
    }

    inline void from_json(json const & j, Hdr& x) {
        x.enable = get_stack_optional<bool>(j, "enable");
    }

    inline void to_json(json & j, Hdr const & x) {
        j = json::object();
        j["enable"] = x.enable;
    }

    inline void from_json(json const & j, ProfileConfigLens& x) {
        x.exchanger = get_stack_optional<bool>(j, "exchanger");
        x.focus = get_stack_optional<int64_t>(j, "focus");
        x.zf_mirror_profile0 = get_stack_optional<bool>(j, "zfMirrorProfile0");
        x.zoom = get_stack_optional<int64_t>(j, "zoom");
    }

    inline void to_json(json & j, ProfileConfigLens const & x) {
        j = json::object();
        j["exchanger"] = x.exchanger;
        j["focus"] = x.focus;
        j["zfMirrorProfile0"] = x.zf_mirror_profile0;
        j["zoom"] = x.zoom;
    }

    inline void from_json(json const & j, MovFilter& x) {
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.only_check = get_stack_optional<bool>(j, "onlyCheck");
        x.roi = get_stack_optional<Roi1Class>(j, "roi");
        x.threshold = get_stack_optional<double>(j, "threshold");
    }

    inline void to_json(json & j, MovFilter const & x) {
        j = json::object();
        j["enabled"] = x.enabled;
        j["onlyCheck"] = x.only_check;
        j["roi"] = x.roi;
        j["threshold"] = x.threshold;
    }

    inline void from_json(json const & j, Power& x) {
        x.out = get_stack_optional<int64_t>(j, "out");
        x.percent = get_stack_optional<int64_t>(j, "percent");
    }

    inline void to_json(json & j, Power const & x) {
        j = json::object();
        j["out"] = x.out;
        j["percent"] = x.percent;
    }

    inline void from_json(json const & j, Flash& x) {
        x.power = get_stack_optional<std::vector<Power>>(j, "power");
    }

    inline void to_json(json & j, Flash const & x) {
        j = json::object();
        j["power"] = x.power;
    }

    inline void from_json(json const & j, SettingGain& x) {
        x.percentage_of_current = get_stack_optional<bool>(j, "percentageOfCurrent");
        x.value = get_stack_optional<double>(j, "value");
    }

    inline void to_json(json & j, SettingGain const & x) {
        j = json::object();
        j["percentageOfCurrent"] = x.percentage_of_current;
        j["value"] = x.value;
    }

    inline void from_json(json const & j, Shutter& x) {
        x.percentage_of_current = get_stack_optional<bool>(j, "percentageOfCurrent");
        x.value = get_stack_optional<double>(j, "value");
    }

    inline void to_json(json & j, Shutter const & x) {
        j = json::object();
        j["percentageOfCurrent"] = x.percentage_of_current;
        j["value"] = x.value;
    }

    inline void from_json(json const & j, Something& x) {
        x.flash = get_stack_optional<Flash>(j, "flash");
        x.gain = get_stack_optional<SettingGain>(j, "gain");
        x.shutter = get_stack_optional<Shutter>(j, "shutter");
    }

    inline void to_json(json & j, Something const & x) {
        j = json::object();
        j["flash"] = x.flash;
        j["gain"] = x.gain;
        j["shutter"] = x.shutter;
    }

    inline void from_json(json const & j, MultipleExposures& x) {
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.settings = get_stack_optional<std::vector<Something>>(j, "settings");
    }

    inline void to_json(json & j, MultipleExposures const & x) {
        j = json::object();
        j["enabled"] = x.enabled;
        j["settings"] = x.settings;
    }

    inline void from_json(json const & j, Overlay& x) {
        x.enable = get_stack_optional<bool>(j, "enable");
        x.text = get_stack_optional<std::string>(j, "text");
    }

    inline void to_json(json & j, Overlay const & x) {
        j = json::object();
        j["enable"] = x.enable;
        j["text"] = x.text;
    }

    inline void from_json(json const & j, Lower& x) {
        x.end_time = get_stack_optional<std::string>(j, "endTime");
        x.hold_time = get_stack_optional<int64_t>(j, "holdTime");
        x.level = get_stack_optional<double>(j, "level");
        x.profile = get_stack_optional<int64_t>(j, "profile");
        x.start_time = get_stack_optional<std::string>(j, "startTime");
    }

    inline void to_json(json & j, Lower const & x) {
        j = json::object();
        j["endTime"] = x.end_time;
        j["holdTime"] = x.hold_time;
        j["level"] = x.level;
        j["profile"] = x.profile;
        j["startTime"] = x.start_time;
    }

    inline void from_json(json const & j, Transitions& x) {
        x.lower = get_stack_optional<Lower>(j, "lower");
        x.upper = get_stack_optional<Lower>(j, "upper");
    }

    inline void to_json(json & j, Transitions const & x) {
        j = json::object();
        j["lower"] = x.lower;
        j["upper"] = x.upper;
    }

    inline void from_json(json const & j, Trigger& x) {
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.event = get_stack_optional<std::string>(j, "event");
        x.minimum_interval = get_stack_optional<int64_t>(j, "minimumInterval");
        x.port = get_stack_optional<int64_t>(j, "port");
        x.roi = get_stack_optional<Roi1Class>(j, "roi");
        x.threshold = get_stack_optional<double>(j, "threshold");
    }

    inline void to_json(json & j, Trigger const & x) {
        j = json::object();
        j["enabled"] = x.enabled;
        j["event"] = x.event;
        j["minimumInterval"] = x.minimum_interval;
        j["port"] = x.port;
        j["roi"] = x.roi;
        j["threshold"] = x.threshold;
    }

    inline void from_json(json const & j, ProfileConfigWhitebalance& x) {
        x.automatic = get_stack_optional<bool>(j, "automatic");
        x.weights = get_stack_optional<WhitebalanceClass>(j, "weights");
    }

    inline void to_json(json & j, ProfileConfigWhitebalance const & x) {
        j = json::object();
        j["automatic"] = x.automatic;
        j["weights"] = x.weights;
    }

    inline void from_json(json const & j, ProfileConfig& x) {
        x.active = get_stack_optional<bool>(j, "active");
        x.advanced = get_stack_optional<Advanced>(j, "advanced");
        x.color = get_stack_optional<Color>(j, "color");
        x.description = get_stack_optional<std::string>(j, "description");
        x.exposure = get_stack_optional<Exposure>(j, "exposure");
        x.hdr = get_stack_optional<Hdr>(j, "hdr");
        x.id = j.at("id").get<int64_t>();
        x.lens = get_stack_optional<ProfileConfigLens>(j, "lens");
        x.mov_filter = get_stack_optional<MovFilter>(j, "movFilter");
        x.multiple_exposures = get_stack_optional<MultipleExposures>(j, "multipleExposures");
        x.name = get_stack_optional<std::string>(j, "name");
        x.overlay = get_stack_optional<Overlay>(j, "overlay");
        x.transitions = get_stack_optional<Transitions>(j, "transitions");
        x.trigger = get_stack_optional<Trigger>(j, "trigger");
        x.whitebalance = get_stack_optional<ProfileConfigWhitebalance>(j, "whitebalance");
    }

    inline void to_json(json & j, ProfileConfig const & x) {
        j = json::object();
        j["active"] = x.active;
        j["advanced"] = x.advanced;
        j["color"] = x.color;
        j["description"] = x.description;
        j["exposure"] = x.exposure;
        j["hdr"] = x.hdr;
        j["id"] = x.id;
        j["lens"] = x.lens;
        j["movFilter"] = x.mov_filter;
        j["multipleExposures"] = x.multiple_exposures;
        j["name"] = x.name;
        j["overlay"] = x.overlay;
        j["transitions"] = x.transitions;
        j["trigger"] = x.trigger;
        j["whitebalance"] = x.whitebalance;
    }

    inline void from_json(json const & j, OcrConfigOcr& x) {
        x.avg_char_height = get_stack_optional<int64_t>(j, "avgCharHeight");
        x.avg_plate_angle = get_stack_optional<double>(j, "avgPlateAngle");
        x.avg_plate_slant = get_stack_optional<double>(j, "avgPlateSlant");
        x.classifier_expansion = get_stack_optional<double>(j, "classifierExpansion");
        x.country_code = get_stack_optional<int64_t>(j, "countryCode");
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.licensed = get_stack_optional<bool>(j, "licensed");
        x.max_char_height = get_stack_optional<int64_t>(j, "maxCharHeight");
        x.max_low_prob_chars = get_stack_optional<int64_t>(j, "maxLowProbChars");
        x.max_plates = get_stack_optional<int64_t>(j, "maxPlates");
        x.min_char_height = get_stack_optional<int64_t>(j, "minCharHeight");
        x.min_prob_per_char = get_stack_optional<double>(j, "minProbPerChar");
        x.processing_mode = get_stack_optional<int64_t>(j, "processingMode");
        x.processing_queue = get_stack_optional<int64_t>(j, "processingQueue");
        x.processing_threads = get_stack_optional<int64_t>(j, "processingThreads");
        x.processing_timeout = get_stack_optional<int64_t>(j, "processingTimeout");
        x.roi = get_stack_optional<Roi1Class>(j, "roi");
        x.use_classifier_result = get_stack_optional<bool>(j, "useClassifierResult");
        x.vehicle_type = get_stack_optional<int64_t>(j, "vehicleType");
    }

    inline void to_json(json & j, OcrConfigOcr const & x) {
        j = json::object();
        j["avgCharHeight"] = x.avg_char_height;
        j["avgPlateAngle"] = x.avg_plate_angle;
        j["avgPlateSlant"] = x.avg_plate_slant;
        j["classifierExpansion"] = x.classifier_expansion;
        j["countryCode"] = x.country_code;
        j["enabled"] = x.enabled;
        j["licensed"] = x.licensed;
        j["maxCharHeight"] = x.max_char_height;
        j["maxLowProbChars"] = x.max_low_prob_chars;
        j["maxPlates"] = x.max_plates;
        j["minCharHeight"] = x.min_char_height;
        j["minProbPerChar"] = x.min_prob_per_char;
        j["processingMode"] = x.processing_mode;
        j["processingQueue"] = x.processing_queue;
        j["processingThreads"] = x.processing_threads;
        j["processingTimeout"] = x.processing_timeout;
        j["roi"] = x.roi;
        j["useClassifierResult"] = x.use_classifier_result;
        j["vehicleType"] = x.vehicle_type;
    }

    inline void from_json(json const & j, OcrConfig& x) {
        x.ocr = get_stack_optional<OcrConfigOcr>(j, "ocr");
    }

    inline void to_json(json & j, OcrConfig const & x) {
        j = json::object();
        j["ocr"] = x.ocr;
    }

    inline void from_json(json const & j, Voting& x) {
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.forward_without_plate_if_tracker = get_stack_optional<bool>(j, "forwardWithoutPlateIfTracker");
        x.keep_best_only = get_stack_optional<bool>(j, "keepBestOnly");
        x.max_diff_chars = get_stack_optional<int64_t>(j, "maxDiffChars");
        x.roi1 = get_stack_optional<Roi1Class>(j, "roi1");
        x.roi2 = get_stack_optional<Roi1Class>(j, "roi2");
        x.same_plate_debounce = get_stack_optional<int64_t>(j, "samePlateDebounce");
        x.use_classifier = get_stack_optional<bool>(j, "useClassifier");
    }

    inline void to_json(json & j, Voting const & x) {
        j = json::object();
        j["enabled"] = x.enabled;
        j["forwardWithoutPlateIfTracker"] = x.forward_without_plate_if_tracker;
        j["keepBestOnly"] = x.keep_best_only;
        j["maxDiffChars"] = x.max_diff_chars;
        j["roi1"] = x.roi1;
        j["roi2"] = x.roi2;
        j["samePlateDebounce"] = x.same_plate_debounce;
        j["useClassifier"] = x.use_classifier;
    }

    inline void from_json(json const & j, AnalyticsConfig& x) {
        x.voting = get_stack_optional<Voting>(j, "voting");
    }

    inline void to_json(json & j, AnalyticsConfig const & x) {
        j = json::object();
        j["voting"] = x.voting;
    }

    inline void from_json(json const & j, SpeedCalibrationRegion1& x) {
        x.p0_top1_sz = get_stack_optional<double>(j, "p0top1sz");
        x.x0 = get_stack_optional<int64_t>(j, "x0");
        x.x1 = get_stack_optional<int64_t>(j, "x1");
        x.x2 = get_stack_optional<int64_t>(j, "x2");
        x.y0 = get_stack_optional<int64_t>(j, "y0");
        x.y1 = get_stack_optional<int64_t>(j, "y1");
        x.y2 = get_stack_optional<int64_t>(j, "y2");
    }

    inline void to_json(json & j, SpeedCalibrationRegion1 const & x) {
        j = json::object();
        j["p0top1sz"] = x.p0_top1_sz;
        j["x0"] = x.x0;
        j["x1"] = x.x1;
        j["x2"] = x.x2;
        j["y0"] = x.y0;
        j["y1"] = x.y1;
        j["y2"] = x.y2;
    }

    inline void from_json(json const & j, TriggerRegion0& x) {
        x.dir = get_stack_optional<std::string>(j, "dir");
        x.x0 = get_stack_optional<int64_t>(j, "x0");
        x.x1 = get_stack_optional<int64_t>(j, "x1");
        x.y0 = get_stack_optional<int64_t>(j, "y0");
        x.y1 = get_stack_optional<int64_t>(j, "y1");
    }

    inline void to_json(json & j, TriggerRegion0 const & x) {
        j = json::object();
        j["dir"] = x.dir;
        j["x0"] = x.x0;
        j["x1"] = x.x1;
        j["y0"] = x.y0;
        j["y1"] = x.y1;
    }

    inline void from_json(json const & j, ClassifierConfigClassifier& x) {
        x.enable_characteristics = get_stack_optional<bool>(j, "enableCharacteristics");
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.enable_speed = get_stack_optional<bool>(j, "enableSpeed");
        x.first_only = get_stack_optional<bool>(j, "firstOnly");
        x.licensed = get_stack_optional<bool>(j, "licensed");
        x.min_probability = get_stack_optional<double>(j, "minProbability");
        x.model_type = get_stack_optional<int64_t>(j, "modelType");
        x.processing_queue = get_stack_optional<int64_t>(j, "processingQueue");
        x.processing_threads = get_stack_optional<int64_t>(j, "processingThreads");
        x.scene_type = get_stack_optional<int64_t>(j, "sceneType");
        x.speed_calibration_region1 = get_stack_optional<SpeedCalibrationRegion1>(j, "speedCalibrationRegion1");
        x.speed_calibration_region2 = get_stack_optional<SpeedCalibrationRegion1>(j, "speedCalibrationRegion2");
        x.trigger_enabled = get_stack_optional<bool>(j, "triggerEnabled");
        x.trigger_region0 = get_stack_optional<TriggerRegion0>(j, "triggerRegion0");
        x.trigger_region1 = get_stack_optional<TriggerRegion0>(j, "triggerRegion1");
        x.trigger_region2 = get_stack_optional<TriggerRegion0>(j, "triggerRegion2");
        x.trigger_region3 = get_stack_optional<TriggerRegion0>(j, "triggerRegion3");
    }

    inline void to_json(json & j, ClassifierConfigClassifier const & x) {
        j = json::object();
        j["enableCharacteristics"] = x.enable_characteristics;
        j["enabled"] = x.enabled;
        j["enableSpeed"] = x.enable_speed;
        j["firstOnly"] = x.first_only;
        j["licensed"] = x.licensed;
        j["minProbability"] = x.min_probability;
        j["modelType"] = x.model_type;
        j["processingQueue"] = x.processing_queue;
        j["processingThreads"] = x.processing_threads;
        j["sceneType"] = x.scene_type;
        j["speedCalibrationRegion1"] = x.speed_calibration_region1;
        j["speedCalibrationRegion2"] = x.speed_calibration_region2;
        j["triggerEnabled"] = x.trigger_enabled;
        j["triggerRegion0"] = x.trigger_region0;
        j["triggerRegion1"] = x.trigger_region1;
        j["triggerRegion2"] = x.trigger_region2;
        j["triggerRegion3"] = x.trigger_region3;
    }

    inline void from_json(json const & j, ClassifierConfig& x) {
        x.classifier = get_stack_optional<ClassifierConfigClassifier>(j, "classifier");
    }

    inline void to_json(json & j, ClassifierConfig const & x) {
        j = json::object();
        j["classifier"] = x.classifier;
    }

    inline void from_json(json const & j, AutoFocusRoi& x) {
        x.center_x = get_stack_optional<int64_t>(j, "centerX");
        x.center_y = get_stack_optional<int64_t>(j, "centerY");
        x.height = get_stack_optional<int64_t>(j, "height");
        x.width = get_stack_optional<int64_t>(j, "width");
    }

    inline void to_json(json & j, AutoFocusRoi const & x) {
        j = json::object();
        j["centerX"] = x.center_x;
        j["centerY"] = x.center_y;
        j["height"] = x.height;
        j["width"] = x.width;
    }

    inline void from_json(json const & j, AutoFocus& x) {
        x.coarse_step = get_stack_optional<int64_t>(j, "coarseStep");
        x.contrast_threshold = get_stack_optional<double>(j, "contrastThreshold");
        x.roi = get_stack_optional<AutoFocusRoi>(j, "roi");
        x.run = get_stack_optional<bool>(j, "run");
        x.update_rate = get_stack_optional<int64_t>(j, "updateRate");
    }

    inline void to_json(json & j, AutoFocus const & x) {
        j = json::object();
        j["coarseStep"] = x.coarse_step;
        j["contrastThreshold"] = x.contrast_threshold;
        j["roi"] = x.roi;
        j["run"] = x.run;
        j["updateRate"] = x.update_rate;
    }

    inline void from_json(json const & j, H264Main& x) {
        x.available = get_stack_optional<bool>(j, "available");
        x.bitrate = get_stack_optional<int64_t>(j, "bitrate");
        x.control_rate = get_stack_optional<std::string>(j, "controlRate");
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.gop = get_stack_optional<int64_t>(j, "gop");
        x.profile = get_stack_optional<std::string>(j, "profile");
        x.running = get_stack_optional<bool>(j, "running");
        x.source = get_stack_optional<std::string>(j, "source");
    }

    inline void to_json(json & j, H264Main const & x) {
        j = json::object();
        j["available"] = x.available;
        j["bitrate"] = x.bitrate;
        j["controlRate"] = x.control_rate;
        j["enabled"] = x.enabled;
        j["gop"] = x.gop;
        j["profile"] = x.profile;
        j["running"] = x.running;
        j["source"] = x.source;
    }

    inline void from_json(json const & j, H264& x) {
        x.available = get_stack_optional<bool>(j, "available");
        x.encoder_type = get_stack_optional<std::string>(j, "encoder_type");
        x.main = get_stack_optional<H264Main>(j, "main");
    }

    inline void to_json(json & j, H264 const & x) {
        j = json::object();
        j["available"] = x.available;
        j["encoder_type"] = x.encoder_type;
        j["main"] = x.main;
    }

    inline void from_json(json const & j, MjpegMain& x) {
        x.available = get_stack_optional<bool>(j, "available");
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.framerate = get_stack_optional<double>(j, "framerate");
        x.quality = get_stack_optional<int64_t>(j, "quality");
    }

    inline void to_json(json & j, MjpegMain const & x) {
        j = json::object();
        j["available"] = x.available;
        j["enabled"] = x.enabled;
        j["framerate"] = x.framerate;
        j["quality"] = x.quality;
    }

    inline void from_json(json const & j, Mjpeg& x) {
        x.available = get_stack_optional<bool>(j, "available");
        x.main = get_stack_optional<MjpegMain>(j, "main");
    }

    inline void to_json(json & j, Mjpeg const & x) {
        j = json::object();
        j["available"] = x.available;
        j["main"] = x.main;
    }

    inline void from_json(json const & j, StreamConfig& x) {
        x.h264 = get_stack_optional<H264>(j, "h264");
        x.mjpeg = get_stack_optional<Mjpeg>(j, "mjpeg");
    }

    inline void to_json(json & j, StreamConfig const & x) {
        j = json::object();
        j["h264"] = x.h264;
        j["mjpeg"] = x.mjpeg;
    }

    inline void from_json(json const & j, Scenario1Crop& x) {
        x.x0 = get_stack_optional<int64_t>(j, "x0");
        x.x1 = get_stack_optional<int64_t>(j, "x1");
        x.y0 = get_stack_optional<int64_t>(j, "y0");
        x.y1 = get_stack_optional<int64_t>(j, "y1");
    }

    inline void to_json(json & j, Scenario1Crop const & x) {
        j = json::object();
        j["x0"] = x.x0;
        j["x1"] = x.x1;
        j["y0"] = x.y0;
        j["y1"] = x.y1;
    }

    inline void from_json(json const & j, Scenario2Crop& x) {
        x.x0 = get_stack_optional<int64_t>(j, "x0");
        x.x1 = get_stack_optional<int64_t>(j, "x1");
        x.y0 = get_stack_optional<int64_t>(j, "y0");
        x.y1 = get_stack_optional<int64_t>(j, "y1");
    }

    inline void to_json(json & j, Scenario2Crop const & x) {
        j = json::object();
        j["x0"] = x.x0;
        j["x1"] = x.x1;
        j["y0"] = x.y0;
        j["y1"] = x.y1;
    }

    inline void from_json(json const & j, SnapshotCrop& x) {
        x.enable = get_stack_optional<bool>(j, "enable");
        x.mode = get_stack_optional<std::string>(j, "mode");
        x.x0 = get_stack_optional<int64_t>(j, "x0");
        x.x1 = get_stack_optional<int64_t>(j, "x1");
        x.y0 = get_stack_optional<int64_t>(j, "y0");
        x.y1 = get_stack_optional<int64_t>(j, "y1");
    }

    inline void to_json(json & j, SnapshotCrop const & x) {
        j = json::object();
        j["enable"] = x.enable;
        j["mode"] = x.mode;
        j["x0"] = x.x0;
        j["x1"] = x.x1;
        j["y0"] = x.y0;
        j["y1"] = x.y1;
    }

    inline void from_json(json const & j, Misc& x) {
        x.camera_orientation = get_stack_optional<bool>(j, "cameraOrientation");
        x.iris_hint = get_stack_optional<std::string>(j, "irisHint");
        x.jpeg_quality = get_stack_optional<int64_t>(j, "jpegQuality");
        x.legacy_tsync_gpio = get_stack_optional<int64_t>(j, "legacyTsyncGpio");
        x.scenario1_crop = get_stack_optional<Scenario1Crop>(j, "scenario1Crop");
        x.scenario1_overlay = get_stack_optional<std::string>(j, "scenario1Overlay");
        x.scenario1_overlay_text_size = get_stack_optional<int64_t>(j, "scenario1OverlayTextSize");
        x.scenario2_crop = get_stack_optional<Scenario2Crop>(j, "scenario2Crop");
        x.scenario2_overlay = get_stack_optional<std::string>(j, "scenario2Overlay");
        x.scenario2_overlay_text_size = get_stack_optional<int64_t>(j, "scenario2OverlayTextSize");
        x.scenario_overlay_color = get_stack_optional<std::string>(j, "scenarioOverlayColor");
        x.snapshot_crop = get_stack_optional<SnapshotCrop>(j, "snapshotCrop");
    }

    inline void to_json(json & j, Misc const & x) {
        j = json::object();
        j["cameraOrientation"] = x.camera_orientation;
        j["irisHint"] = x.iris_hint;
        j["jpegQuality"] = x.jpeg_quality;
        j["legacyTsyncGpio"] = x.legacy_tsync_gpio;
        j["scenario1Crop"] = x.scenario1_crop;
        j["scenario1Overlay"] = x.scenario1_overlay;
        j["scenario1OverlayTextSize"] = x.scenario1_overlay_text_size;
        j["scenario2Crop"] = x.scenario2_crop;
        j["scenario2Overlay"] = x.scenario2_overlay;
        j["scenario2OverlayTextSize"] = x.scenario2_overlay_text_size;
        j["scenarioOverlayColor"] = x.scenario_overlay_color;
        j["snapshotCrop"] = x.snapshot_crop;
    }

    inline void from_json(json const & j, Ae& x) {
        x.ctrl_mode = get_stack_optional<std::string>(j, "ctrlMode");
        x.last_run = get_stack_optional<int64_t>(j, "lastRun");
        x.level = get_stack_optional<double>(j, "level");
    }

    inline void to_json(json & j, Ae const & x) {
        j = json::object();
        j["ctrlMode"] = x.ctrl_mode;
        j["lastRun"] = x.last_run;
        j["level"] = x.level;
    }

    inline void from_json(json const & j, Fps& x) {
        x.mjpeg = get_stack_optional<double>(j, "mjpeg");
    }

    inline void to_json(json & j, Fps const & x) {
        j = json::object();
        j["mjpeg"] = x.mjpeg;
    }

    inline void from_json(json const & j, Gps& x) {
        x.altitude = get_stack_optional<double>(j, "altitude");
        x.available = get_stack_optional<bool>(j, "available");
        x.bearing = get_stack_optional<double>(j, "bearing");
        x.dop = get_stack_optional<double>(j, "dop");
        x.fix = get_stack_optional<std::string>(j, "fix");
        x.latitude = get_stack_optional<double>(j, "latitude");
        x.longitude = get_stack_optional<double>(j, "longitude");
        x.num_satellites = get_stack_optional<int64_t>(j, "numSatellites");
        x.seconds_since_last_fix = get_stack_optional<int64_t>(j, "secondsSinceLastFix");
        x.speed = get_stack_optional<double>(j, "speed");
        x.time = get_stack_optional<int64_t>(j, "time");
    }

    inline void to_json(json & j, Gps const & x) {
        j = json::object();
        j["altitude"] = x.altitude;
        j["available"] = x.available;
        j["bearing"] = x.bearing;
        j["dop"] = x.dop;
        j["fix"] = x.fix;
        j["latitude"] = x.latitude;
        j["longitude"] = x.longitude;
        j["numSatellites"] = x.num_satellites;
        j["secondsSinceLastFix"] = x.seconds_since_last_fix;
        j["speed"] = x.speed;
        j["time"] = x.time;
    }

    inline void from_json(json const & j, Isp& x) {
        x.free_buffers = get_stack_optional<int64_t>(j, "freeBuffers");
        x.gain = get_stack_optional<int64_t>(j, "gain");
        x.iris = get_stack_optional<int64_t>(j, "iris");
        x.iris_model = get_stack_optional<std::string>(j, "irisModel");
        x.shutter = get_stack_optional<int64_t>(j, "shutter");
    }

    inline void to_json(json & j, Isp const & x) {
        j = json::object();
        j["freeBuffers"] = x.free_buffers;
        j["gain"] = x.gain;
        j["iris"] = x.iris;
        j["irisModel"] = x.iris_model;
        j["shutter"] = x.shutter;
    }

    inline void from_json(json const & j, MiscVolatileLens& x) {
        x.focus = get_stack_optional<int64_t>(j, "focus");
        x.zoom = get_stack_optional<int64_t>(j, "zoom");
    }

    inline void to_json(json & j, MiscVolatileLens const & x) {
        j = json::object();
        j["focus"] = x.focus;
        j["zoom"] = x.zoom;
    }

    inline void from_json(json const & j, Profile& x) {
        x.id = get_stack_optional<int64_t>(j, "id");
        x.name = get_stack_optional<std::string>(j, "name");
    }

    inline void to_json(json & j, Profile const & x) {
        j = json::object();
        j["id"] = x.id;
        j["name"] = x.name;
    }

    inline void from_json(json const & j, MiscVolatile& x) {
        x.ae = get_stack_optional<Ae>(j, "ae");
        x.fps = get_stack_optional<Fps>(j, "fps");
        x.gps = get_stack_optional<Gps>(j, "gps");
        x.isp = get_stack_optional<Isp>(j, "isp");
        x.lens = get_stack_optional<MiscVolatileLens>(j, "lens");
        x.profile = get_stack_optional<Profile>(j, "profile");
        x.whitebalance = get_stack_optional<WhitebalanceClass>(j, "whitebalance");
    }

    inline void to_json(json & j, MiscVolatile const & x) {
        j = json::object();
        j["ae"] = x.ae;
        j["fps"] = x.fps;
        j["gps"] = x.gps;
        j["isp"] = x.isp;
        j["lens"] = x.lens;
        j["profile"] = x.profile;
        j["whitebalance"] = x.whitebalance;
    }

    inline void from_json(json const & j, Itscampro& x) {
        x.address = get_stack_optional<std::string>(j, "address");
        x.debug = get_stack_optional<bool>(j, "debug");
        x.enable = get_stack_optional<bool>(j, "enable");
        x.port = get_stack_optional<int64_t>(j, "port");
    }

    inline void to_json(json & j, Itscampro const & x) {
        j = json::object();
        j["address"] = x.address;
        j["debug"] = x.debug;
        j["enable"] = x.enable;
        j["port"] = x.port;
    }

    inline void from_json(json const & j, ItscamproConfig& x) {
        x.itscampro = get_stack_optional<Itscampro>(j, "itscampro");
    }

    inline void to_json(json & j, ItscamproConfig const & x) {
        j = json::object();
        j["itscampro"] = x.itscampro;
    }

    inline void from_json(json const & j, ItscamproStatus& x) {
        x.status = get_stack_optional<std::string>(j, "status");
    }

    inline void to_json(json & j, ItscamproStatus const & x) {
        j = json::object();
        j["status"] = x.status;
    }

    inline void from_json(json const & j, Sign& x) {
        x.append_mode = get_stack_optional<std::string>(j, "appendMode");
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.loaded = get_stack_optional<bool>(j, "loaded");
        x.update = get_stack_optional<bool>(j, "update");
    }

    inline void to_json(json & j, Sign const & x) {
        j = json::object();
        j["appendMode"] = x.append_mode;
        j["enabled"] = x.enabled;
        j["loaded"] = x.loaded;
        j["update"] = x.update;
    }

    inline void from_json(json const & j, ImageSignConfig& x) {
        x.sign = get_stack_optional<Sign>(j, "sign");
    }

    inline void to_json(json & j, ImageSignConfig const & x) {
        j = json::object();
        j["sign"] = x.sign;
    }

    inline void from_json(json const & j, Local& x) {
        x.buffer_size_kb = get_stack_optional<int64_t>(j, "bufferSizeKb");
        x.ttl = get_stack_optional<int64_t>(j, "ttl");
    }

    inline void to_json(json & j, Local const & x) {
        j = json::object();
        j["bufferSizeKb"] = x.buffer_size_kb;
        j["ttl"] = x.ttl;
    }

    inline void from_json(json const & j, Transfer& x) {
        x.poll_interval = get_stack_optional<int64_t>(j, "pollInterval");
        x.timeout = get_stack_optional<int64_t>(j, "timeout");
    }

    inline void to_json(json & j, Transfer const & x) {
        j = json::object();
        j["pollInterval"] = x.poll_interval;
        j["timeout"] = x.timeout;
    }

    inline void from_json(json const & j, Ftp& x) {
        x.address = get_stack_optional<std::string>(j, "address");
        x.anonymous = get_stack_optional<bool>(j, "anonymous");
        x.enable = get_stack_optional<bool>(j, "enable");
        x.filename = get_stack_optional<std::string>(j, "filename");
        x.local = get_stack_optional<Local>(j, "local");
        x.password = get_stack_optional<std::string>(j, "password");
        x.port = get_stack_optional<int64_t>(j, "port");
        x.protocol = get_stack_optional<std::string>(j, "protocol");
        x.quality = get_stack_optional<int64_t>(j, "quality");
        x.transfer = get_stack_optional<Transfer>(j, "transfer");
        x.username = get_stack_optional<std::string>(j, "username");
    }

    inline void to_json(json & j, Ftp const & x) {
        j = json::object();
        j["address"] = x.address;
        j["anonymous"] = x.anonymous;
        j["enable"] = x.enable;
        j["filename"] = x.filename;
        j["local"] = x.local;
        j["password"] = x.password;
        j["port"] = x.port;
        j["protocol"] = x.protocol;
        j["quality"] = x.quality;
        j["transfer"] = x.transfer;
        j["username"] = x.username;
    }

    inline void from_json(json const & j, FtpConfig& x) {
        x.ftp = get_stack_optional<Ftp>(j, "ftp");
    }

    inline void to_json(json & j, FtpConfig const & x) {
        j = json::object();
        j["ftp"] = x.ftp;
    }

    inline void from_json(json const & j, LinceConfig& x) {
        x.auth_code = get_stack_optional<std::string>(j, "authCode");
        x.client_endpoint = get_stack_optional<std::string>(j, "clientEndpoint");
        x.client_id = get_stack_optional<std::string>(j, "clientId");
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.environment = get_stack_optional<std::string>(j, "environment");
        x.send_recs_none = get_stack_optional<bool>(j, "sendRecsNone");
        x.timeout_response = get_stack_optional<int64_t>(j, "timeoutResponse");
    }

    inline void to_json(json & j, LinceConfig const & x) {
        j = json::object();
        j["authCode"] = x.auth_code;
        j["clientEndpoint"] = x.client_endpoint;
        j["clientId"] = x.client_id;
        j["enabled"] = x.enabled;
        j["environment"] = x.environment;
        j["sendRecsNone"] = x.send_recs_none;
        j["timeoutResponse"] = x.timeout_response;
    }

    inline void from_json(json const & j, LinceStatus& x) {
        x.lince_status = get_stack_optional<std::string>(j, "linceStatus");
    }

    inline void to_json(json & j, LinceStatus const & x) {
        j = json::object();
        j["linceStatus"] = x.lince_status;
    }

    inline void from_json(json const & j, VehicleIndicator& x) {
        x.vehicle_counter_active_high = get_stack_optional<bool>(j, "vehicleCounterActiveHigh");
        x.vehicle_counter_enabled = get_stack_optional<bool>(j, "vehicleCounterEnabled");
        x.vehicle_counter_gpio = get_stack_optional<int64_t>(j, "vehicleCounterGpio");
        x.vehicle_counter_pulse_width_ms = get_stack_optional<int64_t>(j, "vehicleCounterPulseWidthMs");
        x.vehicle_counter_type = get_stack_optional<int64_t>(j, "vehicleCounterType");
        x.vehicle_counter_udp_port = get_stack_optional<int64_t>(j, "vehicleCounterUdpPort");
        x.vehicle_counter_udp_sample_time_ms = get_stack_optional<int64_t>(j, "vehicleCounterUdpSampleTimeMs");
        x.vehicle_counter_udp_server = get_stack_optional<std::string>(j, "vehicleCounterUdpServer");
    }

    inline void to_json(json & j, VehicleIndicator const & x) {
        j = json::object();
        j["vehicleCounterActiveHigh"] = x.vehicle_counter_active_high;
        j["vehicleCounterEnabled"] = x.vehicle_counter_enabled;
        j["vehicleCounterGpio"] = x.vehicle_counter_gpio;
        j["vehicleCounterPulseWidthMs"] = x.vehicle_counter_pulse_width_ms;
        j["vehicleCounterType"] = x.vehicle_counter_type;
        j["vehicleCounterUdpPort"] = x.vehicle_counter_udp_port;
        j["vehicleCounterUdpSampleTimeMs"] = x.vehicle_counter_udp_sample_time_ms;
        j["vehicleCounterUdpServer"] = x.vehicle_counter_udp_server;
    }

    inline void from_json(json const & j, VehicleIndicatorConfig& x) {
        x.vehicle_indicator = get_stack_optional<VehicleIndicator>(j, "vehicleIndicator");
    }

    inline void to_json(json & j, VehicleIndicatorConfig const & x) {
        j = json::object();
        j["vehicleIndicator"] = x.vehicle_indicator;
    }

    inline void from_json(json const & j, ConfigCgi& x) {
        x.block_api = get_stack_optional<bool>(j, "blockAPI");
    }

    inline void to_json(json & j, ConfigCgi const & x) {
        j = json::object();
        j["blockAPI"] = x.block_api;
    }

    inline void from_json(json const & j, Auth& x) {
        x.password = get_stack_optional<std::string>(j, "password");
        x.require = get_stack_optional<bool>(j, "require");
    }

    inline void to_json(json & j, Auth const & x) {
        j = json::object();
        j["password"] = x.password;
        j["require"] = x.require;
    }

    inline void from_json(json const & j, Cougar& x) {
        x.auth = get_stack_optional<Auth>(j, "auth");
    }

    inline void to_json(json & j, Cougar const & x) {
        j = json::object();
        j["auth"] = x.auth;
    }

    inline void from_json(json const & j, Itscamprotocol& x) {
        x.legacy_mode = get_stack_optional<bool>(j, "legacyMode");
    }

    inline void to_json(json & j, Itscamprotocol const & x) {
        j = json::object();
        j["legacyMode"] = x.legacy_mode;
    }

    inline void from_json(json const & j, ProtocolsConfig& x) {
        x.config_cgi = get_stack_optional<ConfigCgi>(j, "configCgi");
        x.cougar = get_stack_optional<Cougar>(j, "cougar");
        x.itscamprotocol = get_stack_optional<Itscamprotocol>(j, "itscamprotocol");
    }

    inline void to_json(json & j, ProtocolsConfig const & x) {
        j = json::object();
        j["configCgi"] = x.config_cgi;
        j["cougar"] = x.cougar;
        j["itscamprotocol"] = x.itscamprotocol;
    }

    inline void from_json(json const & j, ProfileTransitioner& x) {
        x.automatic = get_stack_optional<bool>(j, "automatic");
        x.level_smoothing = get_stack_optional<std::string>(j, "levelSmoothing");
        x.reset_profiles = get_stack_optional<std::string>(j, "resetProfiles");
        x.smoothing_time = get_stack_optional<int64_t>(j, "smoothingTime");
    }

    inline void to_json(json & j, ProfileTransitioner const & x) {
        j = json::object();
        j["automatic"] = x.automatic;
        j["levelSmoothing"] = x.level_smoothing;
        j["resetProfiles"] = x.reset_profiles;
        j["smoothingTime"] = x.smoothing_time;
    }

    inline void from_json(json const & j, Region0& x) {
        x.name = get_stack_optional<std::string>(j, "name");
        x.x0 = get_stack_optional<int64_t>(j, "x0");
        x.x1 = get_stack_optional<int64_t>(j, "x1");
        x.x2 = get_stack_optional<int64_t>(j, "x2");
        x.x3 = get_stack_optional<int64_t>(j, "x3");
        x.y0 = get_stack_optional<int64_t>(j, "y0");
        x.y1 = get_stack_optional<int64_t>(j, "y1");
        x.y2 = get_stack_optional<int64_t>(j, "y2");
        x.y3 = get_stack_optional<int64_t>(j, "y3");
    }

    inline void to_json(json & j, Region0 const & x) {
        j = json::object();
        j["name"] = x.name;
        j["x0"] = x.x0;
        j["x1"] = x.x1;
        j["x2"] = x.x2;
        j["x3"] = x.x3;
        j["y0"] = x.y0;
        j["y1"] = x.y1;
        j["y2"] = x.y2;
        j["y3"] = x.y3;
    }

    inline void from_json(json const & j, LanesConfig& x) {
        x.enabled = get_stack_optional<bool>(j, "enabled");
        x.region0 = get_stack_optional<Region0>(j, "region0");
        x.region1 = get_stack_optional<Region0>(j, "region1");
        x.region2 = get_stack_optional<Region0>(j, "region2");
    }

    inline void to_json(json & j, LanesConfig const & x) {
        j = json::object();
        j["enabled"] = x.enabled;
        j["region0"] = x.region0;
        j["region1"] = x.region1;
        j["region2"] = x.region2;
    }

    inline void from_json(json const & j, IoConfig& x) {
        x.can_flash = get_stack_optional<bool>(j, "canFlash");
        x.can_trigger = get_stack_optional<bool>(j, "canTrigger");
        x.early_us = get_stack_optional<int64_t>(j, "earlyUs");
        x.group = get_stack_optional<std::string>(j, "group");
        x.identifier = get_stack_optional<std::string>(j, "identifier");
        x.is_input = get_stack_optional<bool>(j, "isInput");
        x.is_on = get_stack_optional<bool>(j, "isOn");
        x.port = j.at("port").get<int64_t>();
        x.protection = get_stack_optional<std::string>(j, "protection");
        x.type = get_stack_optional<std::string>(j, "type");
    }

    inline void to_json(json & j, IoConfig const & x) {
        j = json::object();
        j["canFlash"] = x.can_flash;
        j["canTrigger"] = x.can_trigger;
        j["earlyUs"] = x.early_us;
        j["group"] = x.group;
        j["identifier"] = x.identifier;
        j["isInput"] = x.is_input;
        j["isOn"] = x.is_on;
        j["port"] = x.port;
        j["protection"] = x.protection;
        j["type"] = x.type;
    }

    inline void from_json(json const & j, IoBasic& x) {
        x.is_input = get_stack_optional<bool>(j, "isInput");
        x.is_on = get_stack_optional<bool>(j, "isOn");
        x.port = j.at("port").get<int64_t>();
    }

    inline void to_json(json & j, IoBasic const & x) {
        j = json::object();
        j["isInput"] = x.is_input;
        j["isOn"] = x.is_on;
        j["port"] = x.port;
    }

    inline void from_json(json const & j, Part& x) {
        x.content = j.at("content").get<std::string>();
        x.name = j.at("name").get<std::string>();
        x.type = j.at("type").get<Type>();
    }

    inline void to_json(json & j, Part const & x) {
        j = json::object();
        j["content"] = x.content;
        j["name"] = x.name;
        j["type"] = x.type;
    }

    inline void from_json(json const & j, Body& x) {
        x.parts = j.at("parts").get<std::vector<Part>>();
        x.variant = j.at("variant").get<Variant>();
    }

    inline void to_json(json & j, Body const & x) {
        j = json::object();
        j["parts"] = x.parts;
        j["variant"] = x.variant;
    }

    inline void from_json(json const & j, Header& x) {
        x.name = j.at("name").get<std::string>();
        x.value = j.at("value").get<std::string>();
    }

    inline void to_json(json & j, Header const & x) {
        j = json::object();
        j["name"] = x.name;
        j["value"] = x.value;
    }

    inline void from_json(json const & j, Resolution& x) {
        x.height = j.at("height").get<int64_t>();
        x.width = j.at("width").get<int64_t>();
    }

    inline void to_json(json & j, Resolution const & x) {
        j = json::object();
        j["height"] = x.height;
        j["width"] = x.width;
    }

    inline void from_json(json const & j, Jpeg& x) {
        x.quality = j.at("quality").get<int64_t>();
        x.resolution = j.at("resolution").get<Resolution>();
    }

    inline void to_json(json & j, Jpeg const & x) {
        j = json::object();
        j["quality"] = x.quality;
        j["resolution"] = x.resolution;
    }

    inline void from_json(json const & j, Persistency& x) {
        x.enabled = j.at("enabled").get<bool>();
        x.max_disk_usage = j.at("maxDiskUsage").get<int64_t>();
        x.max_file_age = j.at("maxFileAge").get<int64_t>();
        x.newest_first = j.at("newestFirst").get<bool>();
    }

    inline void to_json(json & j, Persistency const & x) {
        j = json::object();
        j["enabled"] = x.enabled;
        j["maxDiskUsage"] = x.max_disk_usage;
        j["maxFileAge"] = x.max_file_age;
        j["newestFirst"] = x.newest_first;
    }

    inline void from_json(json const & j, Url& x) {
        x.host = j.at("host").get<std::string>();
        x.path = j.at("path").get<std::string>();
        x.query = j.at("query").get<std::vector<std::string>>();
        x.scheme = j.at("scheme").get<Scheme>();
    }

    inline void to_json(json & j, Url const & x) {
        j = json::object();
        j["host"] = x.host;
        j["path"] = x.path;
        j["query"] = x.query;
        j["scheme"] = x.scheme;
    }

    inline void from_json(json const & j, RestApiClientConfig& x) {
        x.body = j.at("body").get<Body>();
        x.enabled = j.at("enabled").get<bool>();
        x.headers = j.at("headers").get<std::vector<Header>>();
        x.jpeg = j.at("jpeg").get<Jpeg>();
        x.method = j.at("method").get<Method>();
        x.persistency = j.at("persistency").get<Persistency>();
        x.retries = j.at("retries").get<int64_t>();
        x.send_individual_requests = j.at("sendIndividualRequests").get<bool>();
        x.send_without_ocr = j.at("sendWithoutOcr").get<bool>();
        x.timeout = j.at("timeout").get<int64_t>();
        x.url = j.at("url").get<Url>();
    }

    inline void to_json(json & j, RestApiClientConfig const & x) {
        j = json::object();
        j["body"] = x.body;
        j["enabled"] = x.enabled;
        j["headers"] = x.headers;
        j["jpeg"] = x.jpeg;
        j["method"] = x.method;
        j["persistency"] = x.persistency;
        j["retries"] = x.retries;
        j["sendIndividualRequests"] = x.send_individual_requests;
        j["sendWithoutOcr"] = x.send_without_ocr;
        j["timeout"] = x.timeout;
        j["url"] = x.url;
    }

    inline void from_json(json const & j, RestApiClientStatus& x) {
        x.code = j.at("code").get<int64_t>();
        x.disk_usage = j.at("diskUsage").get<int64_t>();
        x.file_count = j.at("fileCount").get<int64_t>();
        x.message = j.at("message").get<std::string>();
    }

    inline void to_json(json & j, RestApiClientStatus const & x) {
        j = json::object();
        j["code"] = x.code;
        j["diskUsage"] = x.disk_usage;
        j["fileCount"] = x.file_count;
        j["message"] = x.message;
    }

    inline void from_json(json const & j, AnalyticsClassifier& x) {
        x.customer = get_stack_optional<std::string>(j, "customer");
        x.max_connections = get_stack_optional<int64_t>(j, "maxConnections");
        x.max_threads = get_stack_optional<int64_t>(j, "maxThreads");
        x.serial = get_stack_optional<std::string>(j, "serial");
        x.sha1 = get_stack_optional<std::string>(j, "sha1");
        x.state = get_stack_optional<int64_t>(j, "state");
        x.ttl = get_stack_optional<int64_t>(j, "ttl");
        x.version = get_stack_optional<std::string>(j, "version");
    }

    inline void to_json(json & j, AnalyticsClassifier const & x) {
        j = json::object();
        j["customer"] = x.customer;
        j["maxConnections"] = x.max_connections;
        j["maxThreads"] = x.max_threads;
        j["serial"] = x.serial;
        j["sha1"] = x.sha1;
        j["state"] = x.state;
        j["ttl"] = x.ttl;
        j["version"] = x.version;
    }

    inline void from_json(json const & j, AnalyticsOcr& x) {
        x.customer = get_stack_optional<std::string>(j, "customer");
        x.max_connections = get_stack_optional<int64_t>(j, "maxConnections");
        x.max_threads = get_stack_optional<int64_t>(j, "maxThreads");
        x.serial = get_stack_optional<std::string>(j, "serial");
        x.sha1 = get_stack_optional<std::string>(j, "sha1");
        x.state = get_stack_optional<int64_t>(j, "state");
        x.ttl = get_stack_optional<int64_t>(j, "ttl");
        x.version = get_stack_optional<std::string>(j, "version");
    }

    inline void to_json(json & j, AnalyticsOcr const & x) {
        j = json::object();
        j["customer"] = x.customer;
        j["maxConnections"] = x.max_connections;
        j["maxThreads"] = x.max_threads;
        j["serial"] = x.serial;
        j["sha1"] = x.sha1;
        j["state"] = x.state;
        j["ttl"] = x.ttl;
        j["version"] = x.version;
    }

    inline void from_json(json const & j, Analytics& x) {
        x.classifier = get_stack_optional<AnalyticsClassifier>(j, "classifier");
        x.ocr = get_stack_optional<AnalyticsOcr>(j, "ocr");
    }

    inline void to_json(json & j, Analytics const & x) {
        j = json::object();
        j["classifier"] = x.classifier;
        j["ocr"] = x.ocr;
    }

    inline void from_json(json const & j, DeviceId& x) {
        x.serial = get_stack_optional<std::string>(j, "serial");
    }

    inline void to_json(json & j, DeviceId const & x) {
        j = json::object();
        j["serial"] = x.serial;
    }

    inline void from_json(json const & j, Licenses& x) {
        x.analytics = get_stack_optional<Analytics>(j, "analytics");
        x.device_id = get_stack_optional<DeviceId>(j, "deviceId");
    }

    inline void to_json(json & j, Licenses const & x) {
        j = json::object();
        j["analytics"] = x.analytics;
        j["deviceId"] = x.device_id;
    }

    inline void from_json(json const & j, Mode & x) {
        if (j == "disabled") x = Mode::DISABLED;
        else if (j == "fast") x = Mode::FAST;
        else if (j == "normal") x = Mode::NORMAL;
        else if (j == "slow") x = Mode::SLOW;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, Mode const & x) {
        switch (x) {
            case Mode::DISABLED: j = "disabled"; break;
            case Mode::FAST: j = "fast"; break;
            case Mode::NORMAL: j = "normal"; break;
            case Mode::SLOW: j = "slow"; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"Mode\": " + std::to_string(static_cast<int>(x)));
        }
    }

    inline void from_json(json const & j, Type & x) {
        if (j == "json") x = Type::JSON;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, Type const & x) {
        switch (x) {
            case Type::JSON: j = "json"; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"Type\": " + std::to_string(static_cast<int>(x)));
        }
    }

    inline void from_json(json const & j, Variant & x) {
        if (j == "multipart") x = Variant::MULTIPART;
        else if (j == "singlepart") x = Variant::SINGLEPART;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, Variant const & x) {
        switch (x) {
            case Variant::MULTIPART: j = "multipart"; break;
            case Variant::SINGLEPART: j = "singlepart"; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"Variant\": " + std::to_string(static_cast<int>(x)));
        }
    }

    inline void from_json(json const & j, Method & x) {
        if (j == "get") x = Method::GET;
        else if (j == "post") x = Method::POST;
        else if (j == "put") x = Method::PUT;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, Method const & x) {
        switch (x) {
            case Method::GET: j = "get"; break;
            case Method::POST: j = "post"; break;
            case Method::PUT: j = "put"; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"Method\": " + std::to_string(static_cast<int>(x)));
        }
    }

    inline void from_json(json const & j, Scheme & x) {
        if (j == "http") x = Scheme::HTTP;
        else if (j == "https") x = Scheme::HTTPS;
        else { throw std::runtime_error("Input JSON does not conform to schema!"); }
    }

    inline void to_json(json & j, Scheme const & x) {
        switch (x) {
            case Scheme::HTTP: j = "http"; break;
            case Scheme::HTTPS: j = "https"; break;
            default: throw std::runtime_error("Unexpected value in enumeration \"Scheme\": " + std::to_string(static_cast<int>(x)));
        }
    }
    // ---- Partial-JSON serializers (generated) ----

    inline json to_partial_json(Exposition const & x) {
        json j = json::object();
        if (x.preferred_shutter) j["preferredShutter"] = *x.preferred_shutter;
        if (x.update_factor) j["updateFactor"] = *x.update_factor;
        if (x.update_rate) j["updateRate"] = *x.update_rate;
        return j;
    }

    inline json to_partial_json(AdvancedIris const & x) {
        json j = json::object();
        if (x.update_rate) j["updateRate"] = *x.update_rate;
        return j;
    }

    inline json to_partial_json(AdvancedWhitebalance const & x) {
        json j = json::object();
        if (x.update_rate) j["updateRate"] = *x.update_rate;
        return j;
    }

    inline json to_partial_json(Advanced const & x) {
        json j = json::object();
        if (x.exposition) j["exposition"] = to_partial_json(*x.exposition);
        if (x.iris) j["iris"] = to_partial_json(*x.iris);
        if (x.whitebalance) j["whitebalance"] = to_partial_json(*x.whitebalance);
        return j;
    }

    inline json to_partial_json(WhitebalanceClass const & x) {
        json j = json::object();
        if (x.blue) j["blue"] = *x.blue;
        if (x.green) j["green"] = *x.green;
        if (x.red) j["red"] = *x.red;
        return j;
    }

    inline json to_partial_json(Color const & x) {
        json j = json::object();
        if (x.blacklevel) j["blacklevel"] = *x.blacklevel;
        if (x.brightness) j["brightness"] = *x.brightness;
        if (x.contrast) j["contrast"] = *x.contrast;
        if (x.gain) j["gain"] = to_partial_json(*x.gain);
        if (x.gamma) j["gamma"] = *x.gamma;
        if (x.saturation) j["saturation"] = *x.saturation;
        return j;
    }

    inline json to_partial_json(ShutterClass const & x) {
        json j = json::object();
        if (x.automatic) j["automatic"] = *x.automatic;
        if (x.fixed_value) j["fixedValue"] = *x.fixed_value;
        if (x.max_value) j["maxValue"] = *x.max_value;
        if (x.min_value) j["minValue"] = *x.min_value;
        return j;
    }

    inline json to_partial_json(ExposureIris const & x) {
        json j = json::object();
        if (x.automatic) j["automatic"] = *x.automatic;
        if (x.fixed_value) j["fixedValue"] = *x.fixed_value;
        return j;
    }

    inline json to_partial_json(Roi1Class const & x) {
        json j = json::object();
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.x0) j["x0"] = *x.x0;
        if (x.x1) j["x1"] = *x.x1;
        if (x.x2) j["x2"] = *x.x2;
        if (x.x3) j["x3"] = *x.x3;
        if (x.y0) j["y0"] = *x.y0;
        if (x.y1) j["y1"] = *x.y1;
        if (x.y2) j["y2"] = *x.y2;
        if (x.y3) j["y3"] = *x.y3;
        return j;
    }

    inline json to_partial_json(Level const & x) {
        json j = json::object();
        if (x.hold_time) j["holdTime"] = *x.hold_time;
        if (x.mode) j["mode"] = *x.mode;
        if (x.roi) j["roi"] = to_partial_json(*x.roi);
        if (x.target_value) j["targetValue"] = *x.target_value;
        if (x.update_rate) j["updateRate"] = *x.update_rate;
        return j;
    }

    inline json to_partial_json(Exposure const & x) {
        json j = json::object();
        if (x.gain) j["gain"] = to_partial_json(*x.gain);
        if (x.iris) j["iris"] = to_partial_json(*x.iris);
        if (x.level) j["level"] = to_partial_json(*x.level);
        if (x.shutter) j["shutter"] = to_partial_json(*x.shutter);
        return j;
    }

    inline json to_partial_json(Hdr const & x) {
        json j = json::object();
        if (x.enable) j["enable"] = *x.enable;
        return j;
    }

    inline json to_partial_json(ProfileConfigLens const & x) {
        json j = json::object();
        if (x.exchanger) j["exchanger"] = *x.exchanger;
        if (x.focus) j["focus"] = *x.focus;
        if (x.zf_mirror_profile0) j["zfMirrorProfile0"] = *x.zf_mirror_profile0;
        if (x.zoom) j["zoom"] = *x.zoom;
        return j;
    }

    inline json to_partial_json(MovFilter const & x) {
        json j = json::object();
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.only_check) j["onlyCheck"] = *x.only_check;
        if (x.roi) j["roi"] = to_partial_json(*x.roi);
        if (x.threshold) j["threshold"] = *x.threshold;
        return j;
    }

    inline json to_partial_json(Power const & x) {
        json j = json::object();
        if (x.out) j["out"] = *x.out;
        if (x.percent) j["percent"] = *x.percent;
        return j;
    }

    inline json to_partial_json(Flash const & x) {
        json j = json::object();
        if (x.power) {
            json arr = json::array();
            for (auto const & e : *x.power) arr.push_back(to_partial_json(e));
            j["power"] = std::move(arr);
        }
        return j;
    }

    inline json to_partial_json(SettingGain const & x) {
        json j = json::object();
        if (x.percentage_of_current) j["percentageOfCurrent"] = *x.percentage_of_current;
        if (x.value) j["value"] = *x.value;
        return j;
    }

    inline json to_partial_json(Shutter const & x) {
        json j = json::object();
        if (x.percentage_of_current) j["percentageOfCurrent"] = *x.percentage_of_current;
        if (x.value) j["value"] = *x.value;
        return j;
    }

    inline json to_partial_json(Something const & x) {
        json j = json::object();
        if (x.flash) j["flash"] = to_partial_json(*x.flash);
        if (x.gain) j["gain"] = to_partial_json(*x.gain);
        if (x.shutter) j["shutter"] = to_partial_json(*x.shutter);
        return j;
    }

    inline json to_partial_json(MultipleExposures const & x) {
        json j = json::object();
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.settings) {
            json arr = json::array();
            for (auto const & e : *x.settings) arr.push_back(to_partial_json(e));
            j["settings"] = std::move(arr);
        }
        return j;
    }

    inline json to_partial_json(Overlay const & x) {
        json j = json::object();
        if (x.enable) j["enable"] = *x.enable;
        if (x.text) j["text"] = *x.text;
        return j;
    }

    inline json to_partial_json(Lower const & x) {
        json j = json::object();
        if (x.end_time) j["endTime"] = *x.end_time;
        if (x.hold_time) j["holdTime"] = *x.hold_time;
        if (x.level) j["level"] = *x.level;
        if (x.profile) j["profile"] = *x.profile;
        if (x.start_time) j["startTime"] = *x.start_time;
        return j;
    }

    inline json to_partial_json(Transitions const & x) {
        json j = json::object();
        if (x.lower) j["lower"] = to_partial_json(*x.lower);
        if (x.upper) j["upper"] = to_partial_json(*x.upper);
        return j;
    }

    inline json to_partial_json(Trigger const & x) {
        json j = json::object();
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.event) j["event"] = *x.event;
        if (x.minimum_interval) j["minimumInterval"] = *x.minimum_interval;
        if (x.port) j["port"] = *x.port;
        if (x.roi) j["roi"] = to_partial_json(*x.roi);
        if (x.threshold) j["threshold"] = *x.threshold;
        return j;
    }

    inline json to_partial_json(ProfileConfigWhitebalance const & x) {
        json j = json::object();
        if (x.automatic) j["automatic"] = *x.automatic;
        if (x.weights) j["weights"] = to_partial_json(*x.weights);
        return j;
    }

    inline json to_partial_json(ProfileConfig const & x) {
        json j = json::object();
        if (x.active) j["active"] = *x.active;
        if (x.advanced) j["advanced"] = to_partial_json(*x.advanced);
        if (x.color) j["color"] = to_partial_json(*x.color);
        if (x.description) j["description"] = *x.description;
        if (x.exposure) j["exposure"] = to_partial_json(*x.exposure);
        if (x.hdr) j["hdr"] = to_partial_json(*x.hdr);
        j["id"] = x.id;
        if (x.lens) j["lens"] = to_partial_json(*x.lens);
        if (x.mov_filter) j["movFilter"] = to_partial_json(*x.mov_filter);
        if (x.multiple_exposures) j["multipleExposures"] = to_partial_json(*x.multiple_exposures);
        if (x.name) j["name"] = *x.name;
        if (x.overlay) j["overlay"] = to_partial_json(*x.overlay);
        if (x.transitions) j["transitions"] = to_partial_json(*x.transitions);
        if (x.trigger) j["trigger"] = to_partial_json(*x.trigger);
        if (x.whitebalance) j["whitebalance"] = to_partial_json(*x.whitebalance);
        return j;
    }

    inline json to_partial_json(OcrConfigOcr const & x) {
        json j = json::object();
        if (x.avg_char_height) j["avgCharHeight"] = *x.avg_char_height;
        if (x.avg_plate_angle) j["avgPlateAngle"] = *x.avg_plate_angle;
        if (x.avg_plate_slant) j["avgPlateSlant"] = *x.avg_plate_slant;
        if (x.classifier_expansion) j["classifierExpansion"] = *x.classifier_expansion;
        if (x.country_code) j["countryCode"] = *x.country_code;
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.licensed) j["licensed"] = *x.licensed;
        if (x.max_char_height) j["maxCharHeight"] = *x.max_char_height;
        if (x.max_low_prob_chars) j["maxLowProbChars"] = *x.max_low_prob_chars;
        if (x.max_plates) j["maxPlates"] = *x.max_plates;
        if (x.min_char_height) j["minCharHeight"] = *x.min_char_height;
        if (x.min_prob_per_char) j["minProbPerChar"] = *x.min_prob_per_char;
        if (x.processing_mode) j["processingMode"] = *x.processing_mode;
        if (x.processing_queue) j["processingQueue"] = *x.processing_queue;
        if (x.processing_threads) j["processingThreads"] = *x.processing_threads;
        if (x.processing_timeout) j["processingTimeout"] = *x.processing_timeout;
        if (x.roi) j["roi"] = to_partial_json(*x.roi);
        if (x.use_classifier_result) j["useClassifierResult"] = *x.use_classifier_result;
        if (x.vehicle_type) j["vehicleType"] = *x.vehicle_type;
        return j;
    }

    inline json to_partial_json(OcrConfig const & x) {
        json j = json::object();
        if (x.ocr) j["ocr"] = to_partial_json(*x.ocr);
        return j;
    }

    inline json to_partial_json(Voting const & x) {
        json j = json::object();
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.forward_without_plate_if_tracker) j["forwardWithoutPlateIfTracker"] = *x.forward_without_plate_if_tracker;
        if (x.keep_best_only) j["keepBestOnly"] = *x.keep_best_only;
        if (x.max_diff_chars) j["maxDiffChars"] = *x.max_diff_chars;
        if (x.roi1) j["roi1"] = to_partial_json(*x.roi1);
        if (x.roi2) j["roi2"] = to_partial_json(*x.roi2);
        if (x.same_plate_debounce) j["samePlateDebounce"] = *x.same_plate_debounce;
        if (x.use_classifier) j["useClassifier"] = *x.use_classifier;
        return j;
    }

    inline json to_partial_json(AnalyticsConfig const & x) {
        json j = json::object();
        if (x.voting) j["voting"] = to_partial_json(*x.voting);
        return j;
    }

    inline json to_partial_json(SpeedCalibrationRegion1 const & x) {
        json j = json::object();
        if (x.p0_top1_sz) j["p0top1sz"] = *x.p0_top1_sz;
        if (x.x0) j["x0"] = *x.x0;
        if (x.x1) j["x1"] = *x.x1;
        if (x.x2) j["x2"] = *x.x2;
        if (x.y0) j["y0"] = *x.y0;
        if (x.y1) j["y1"] = *x.y1;
        if (x.y2) j["y2"] = *x.y2;
        return j;
    }

    inline json to_partial_json(TriggerRegion0 const & x) {
        json j = json::object();
        if (x.dir) j["dir"] = *x.dir;
        if (x.x0) j["x0"] = *x.x0;
        if (x.x1) j["x1"] = *x.x1;
        if (x.y0) j["y0"] = *x.y0;
        if (x.y1) j["y1"] = *x.y1;
        return j;
    }

    inline json to_partial_json(ClassifierConfigClassifier const & x) {
        json j = json::object();
        if (x.enable_characteristics) j["enableCharacteristics"] = *x.enable_characteristics;
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.enable_speed) j["enableSpeed"] = *x.enable_speed;
        if (x.first_only) j["firstOnly"] = *x.first_only;
        if (x.licensed) j["licensed"] = *x.licensed;
        if (x.min_probability) j["minProbability"] = *x.min_probability;
        if (x.model_type) j["modelType"] = *x.model_type;
        if (x.processing_queue) j["processingQueue"] = *x.processing_queue;
        if (x.processing_threads) j["processingThreads"] = *x.processing_threads;
        if (x.scene_type) j["sceneType"] = *x.scene_type;
        if (x.speed_calibration_region1) j["speedCalibrationRegion1"] = to_partial_json(*x.speed_calibration_region1);
        if (x.speed_calibration_region2) j["speedCalibrationRegion2"] = to_partial_json(*x.speed_calibration_region2);
        if (x.trigger_enabled) j["triggerEnabled"] = *x.trigger_enabled;
        if (x.trigger_region0) j["triggerRegion0"] = to_partial_json(*x.trigger_region0);
        if (x.trigger_region1) j["triggerRegion1"] = to_partial_json(*x.trigger_region1);
        if (x.trigger_region2) j["triggerRegion2"] = to_partial_json(*x.trigger_region2);
        if (x.trigger_region3) j["triggerRegion3"] = to_partial_json(*x.trigger_region3);
        return j;
    }

    inline json to_partial_json(ClassifierConfig const & x) {
        json j = json::object();
        if (x.classifier) j["classifier"] = to_partial_json(*x.classifier);
        return j;
    }

    inline json to_partial_json(AutoFocusRoi const & x) {
        json j = json::object();
        if (x.center_x) j["centerX"] = *x.center_x;
        if (x.center_y) j["centerY"] = *x.center_y;
        if (x.height) j["height"] = *x.height;
        if (x.width) j["width"] = *x.width;
        return j;
    }

    inline json to_partial_json(AutoFocus const & x) {
        json j = json::object();
        if (x.coarse_step) j["coarseStep"] = *x.coarse_step;
        if (x.contrast_threshold) j["contrastThreshold"] = *x.contrast_threshold;
        if (x.roi) j["roi"] = to_partial_json(*x.roi);
        if (x.run) j["run"] = *x.run;
        if (x.update_rate) j["updateRate"] = *x.update_rate;
        return j;
    }

    inline json to_partial_json(H264Main const & x) {
        json j = json::object();
        if (x.available) j["available"] = *x.available;
        if (x.bitrate) j["bitrate"] = *x.bitrate;
        if (x.control_rate) j["controlRate"] = *x.control_rate;
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.gop) j["gop"] = *x.gop;
        if (x.profile) j["profile"] = *x.profile;
        if (x.running) j["running"] = *x.running;
        if (x.source) j["source"] = *x.source;
        return j;
    }

    inline json to_partial_json(H264 const & x) {
        json j = json::object();
        if (x.available) j["available"] = *x.available;
        if (x.encoder_type) j["encoder_type"] = *x.encoder_type;
        if (x.main) j["main"] = to_partial_json(*x.main);
        return j;
    }

    inline json to_partial_json(MjpegMain const & x) {
        json j = json::object();
        if (x.available) j["available"] = *x.available;
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.framerate) j["framerate"] = *x.framerate;
        if (x.quality) j["quality"] = *x.quality;
        return j;
    }

    inline json to_partial_json(Mjpeg const & x) {
        json j = json::object();
        if (x.available) j["available"] = *x.available;
        if (x.main) j["main"] = to_partial_json(*x.main);
        return j;
    }

    inline json to_partial_json(StreamConfig const & x) {
        json j = json::object();
        if (x.h264) j["h264"] = to_partial_json(*x.h264);
        if (x.mjpeg) j["mjpeg"] = to_partial_json(*x.mjpeg);
        return j;
    }

    inline json to_partial_json(Scenario1Crop const & x) {
        json j = json::object();
        if (x.x0) j["x0"] = *x.x0;
        if (x.x1) j["x1"] = *x.x1;
        if (x.y0) j["y0"] = *x.y0;
        if (x.y1) j["y1"] = *x.y1;
        return j;
    }

    inline json to_partial_json(Scenario2Crop const & x) {
        json j = json::object();
        if (x.x0) j["x0"] = *x.x0;
        if (x.x1) j["x1"] = *x.x1;
        if (x.y0) j["y0"] = *x.y0;
        if (x.y1) j["y1"] = *x.y1;
        return j;
    }

    inline json to_partial_json(SnapshotCrop const & x) {
        json j = json::object();
        if (x.enable) j["enable"] = *x.enable;
        if (x.mode) j["mode"] = *x.mode;
        if (x.x0) j["x0"] = *x.x0;
        if (x.x1) j["x1"] = *x.x1;
        if (x.y0) j["y0"] = *x.y0;
        if (x.y1) j["y1"] = *x.y1;
        return j;
    }

    inline json to_partial_json(Misc const & x) {
        json j = json::object();
        if (x.camera_orientation) j["cameraOrientation"] = *x.camera_orientation;
        if (x.iris_hint) j["irisHint"] = *x.iris_hint;
        if (x.jpeg_quality) j["jpegQuality"] = *x.jpeg_quality;
        if (x.legacy_tsync_gpio) j["legacyTsyncGpio"] = *x.legacy_tsync_gpio;
        if (x.scenario1_crop) j["scenario1Crop"] = to_partial_json(*x.scenario1_crop);
        if (x.scenario1_overlay) j["scenario1Overlay"] = *x.scenario1_overlay;
        if (x.scenario1_overlay_text_size) j["scenario1OverlayTextSize"] = *x.scenario1_overlay_text_size;
        if (x.scenario2_crop) j["scenario2Crop"] = to_partial_json(*x.scenario2_crop);
        if (x.scenario2_overlay) j["scenario2Overlay"] = *x.scenario2_overlay;
        if (x.scenario2_overlay_text_size) j["scenario2OverlayTextSize"] = *x.scenario2_overlay_text_size;
        if (x.scenario_overlay_color) j["scenarioOverlayColor"] = *x.scenario_overlay_color;
        if (x.snapshot_crop) j["snapshotCrop"] = to_partial_json(*x.snapshot_crop);
        return j;
    }

    inline json to_partial_json(Ae const & x) {
        json j = json::object();
        if (x.ctrl_mode) j["ctrlMode"] = *x.ctrl_mode;
        if (x.last_run) j["lastRun"] = *x.last_run;
        if (x.level) j["level"] = *x.level;
        return j;
    }

    inline json to_partial_json(Fps const & x) {
        json j = json::object();
        if (x.mjpeg) j["mjpeg"] = *x.mjpeg;
        return j;
    }

    inline json to_partial_json(Gps const & x) {
        json j = json::object();
        if (x.altitude) j["altitude"] = *x.altitude;
        if (x.available) j["available"] = *x.available;
        if (x.bearing) j["bearing"] = *x.bearing;
        if (x.dop) j["dop"] = *x.dop;
        if (x.fix) j["fix"] = *x.fix;
        if (x.latitude) j["latitude"] = *x.latitude;
        if (x.longitude) j["longitude"] = *x.longitude;
        if (x.num_satellites) j["numSatellites"] = *x.num_satellites;
        if (x.seconds_since_last_fix) j["secondsSinceLastFix"] = *x.seconds_since_last_fix;
        if (x.speed) j["speed"] = *x.speed;
        if (x.time) j["time"] = *x.time;
        return j;
    }

    inline json to_partial_json(Isp const & x) {
        json j = json::object();
        if (x.free_buffers) j["freeBuffers"] = *x.free_buffers;
        if (x.gain) j["gain"] = *x.gain;
        if (x.iris) j["iris"] = *x.iris;
        if (x.iris_model) j["irisModel"] = *x.iris_model;
        if (x.shutter) j["shutter"] = *x.shutter;
        return j;
    }

    inline json to_partial_json(MiscVolatileLens const & x) {
        json j = json::object();
        if (x.focus) j["focus"] = *x.focus;
        if (x.zoom) j["zoom"] = *x.zoom;
        return j;
    }

    inline json to_partial_json(Profile const & x) {
        json j = json::object();
        if (x.id) j["id"] = *x.id;
        if (x.name) j["name"] = *x.name;
        return j;
    }

    inline json to_partial_json(MiscVolatile const & x) {
        json j = json::object();
        if (x.ae) j["ae"] = to_partial_json(*x.ae);
        if (x.fps) j["fps"] = to_partial_json(*x.fps);
        if (x.gps) j["gps"] = to_partial_json(*x.gps);
        if (x.isp) j["isp"] = to_partial_json(*x.isp);
        if (x.lens) j["lens"] = to_partial_json(*x.lens);
        if (x.profile) j["profile"] = to_partial_json(*x.profile);
        if (x.whitebalance) j["whitebalance"] = to_partial_json(*x.whitebalance);
        return j;
    }

    inline json to_partial_json(Itscampro const & x) {
        json j = json::object();
        if (x.address) j["address"] = *x.address;
        if (x.debug) j["debug"] = *x.debug;
        if (x.enable) j["enable"] = *x.enable;
        if (x.port) j["port"] = *x.port;
        return j;
    }

    inline json to_partial_json(ItscamproConfig const & x) {
        json j = json::object();
        if (x.itscampro) j["itscampro"] = to_partial_json(*x.itscampro);
        return j;
    }

    inline json to_partial_json(ItscamproStatus const & x) {
        json j = json::object();
        if (x.status) j["status"] = *x.status;
        return j;
    }

    inline json to_partial_json(Sign const & x) {
        json j = json::object();
        if (x.append_mode) j["appendMode"] = *x.append_mode;
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.loaded) j["loaded"] = *x.loaded;
        if (x.update) j["update"] = *x.update;
        return j;
    }

    inline json to_partial_json(ImageSignConfig const & x) {
        json j = json::object();
        if (x.sign) j["sign"] = to_partial_json(*x.sign);
        return j;
    }

    inline json to_partial_json(Local const & x) {
        json j = json::object();
        if (x.buffer_size_kb) j["bufferSizeKb"] = *x.buffer_size_kb;
        if (x.ttl) j["ttl"] = *x.ttl;
        return j;
    }

    inline json to_partial_json(Transfer const & x) {
        json j = json::object();
        if (x.poll_interval) j["pollInterval"] = *x.poll_interval;
        if (x.timeout) j["timeout"] = *x.timeout;
        return j;
    }

    inline json to_partial_json(Ftp const & x) {
        json j = json::object();
        if (x.address) j["address"] = *x.address;
        if (x.anonymous) j["anonymous"] = *x.anonymous;
        if (x.enable) j["enable"] = *x.enable;
        if (x.filename) j["filename"] = *x.filename;
        if (x.local) j["local"] = to_partial_json(*x.local);
        if (x.password) j["password"] = *x.password;
        if (x.port) j["port"] = *x.port;
        if (x.protocol) j["protocol"] = *x.protocol;
        if (x.quality) j["quality"] = *x.quality;
        if (x.transfer) j["transfer"] = to_partial_json(*x.transfer);
        if (x.username) j["username"] = *x.username;
        return j;
    }

    inline json to_partial_json(FtpConfig const & x) {
        json j = json::object();
        if (x.ftp) j["ftp"] = to_partial_json(*x.ftp);
        return j;
    }

    inline json to_partial_json(LinceConfig const & x) {
        json j = json::object();
        if (x.auth_code) j["authCode"] = *x.auth_code;
        if (x.client_endpoint) j["clientEndpoint"] = *x.client_endpoint;
        if (x.client_id) j["clientId"] = *x.client_id;
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.environment) j["environment"] = *x.environment;
        if (x.send_recs_none) j["sendRecsNone"] = *x.send_recs_none;
        if (x.timeout_response) j["timeoutResponse"] = *x.timeout_response;
        return j;
    }

    inline json to_partial_json(LinceStatus const & x) {
        json j = json::object();
        if (x.lince_status) j["linceStatus"] = *x.lince_status;
        return j;
    }

    inline json to_partial_json(VehicleIndicator const & x) {
        json j = json::object();
        if (x.vehicle_counter_active_high) j["vehicleCounterActiveHigh"] = *x.vehicle_counter_active_high;
        if (x.vehicle_counter_enabled) j["vehicleCounterEnabled"] = *x.vehicle_counter_enabled;
        if (x.vehicle_counter_gpio) j["vehicleCounterGpio"] = *x.vehicle_counter_gpio;
        if (x.vehicle_counter_pulse_width_ms) j["vehicleCounterPulseWidthMs"] = *x.vehicle_counter_pulse_width_ms;
        if (x.vehicle_counter_type) j["vehicleCounterType"] = *x.vehicle_counter_type;
        if (x.vehicle_counter_udp_port) j["vehicleCounterUdpPort"] = *x.vehicle_counter_udp_port;
        if (x.vehicle_counter_udp_sample_time_ms) j["vehicleCounterUdpSampleTimeMs"] = *x.vehicle_counter_udp_sample_time_ms;
        if (x.vehicle_counter_udp_server) j["vehicleCounterUdpServer"] = *x.vehicle_counter_udp_server;
        return j;
    }

    inline json to_partial_json(VehicleIndicatorConfig const & x) {
        json j = json::object();
        if (x.vehicle_indicator) j["vehicleIndicator"] = to_partial_json(*x.vehicle_indicator);
        return j;
    }

    inline json to_partial_json(ConfigCgi const & x) {
        json j = json::object();
        if (x.block_api) j["blockAPI"] = *x.block_api;
        return j;
    }

    inline json to_partial_json(Auth const & x) {
        json j = json::object();
        if (x.password) j["password"] = *x.password;
        if (x.require) j["require"] = *x.require;
        return j;
    }

    inline json to_partial_json(Cougar const & x) {
        json j = json::object();
        if (x.auth) j["auth"] = to_partial_json(*x.auth);
        return j;
    }

    inline json to_partial_json(Itscamprotocol const & x) {
        json j = json::object();
        if (x.legacy_mode) j["legacyMode"] = *x.legacy_mode;
        return j;
    }

    inline json to_partial_json(ProtocolsConfig const & x) {
        json j = json::object();
        if (x.config_cgi) j["configCgi"] = to_partial_json(*x.config_cgi);
        if (x.cougar) j["cougar"] = to_partial_json(*x.cougar);
        if (x.itscamprotocol) j["itscamprotocol"] = to_partial_json(*x.itscamprotocol);
        return j;
    }

    inline json to_partial_json(ProfileTransitioner const & x) {
        json j = json::object();
        if (x.automatic) j["automatic"] = *x.automatic;
        if (x.level_smoothing) j["levelSmoothing"] = *x.level_smoothing;
        if (x.reset_profiles) j["resetProfiles"] = *x.reset_profiles;
        if (x.smoothing_time) j["smoothingTime"] = *x.smoothing_time;
        return j;
    }

    inline json to_partial_json(Region0 const & x) {
        json j = json::object();
        if (x.name) j["name"] = *x.name;
        if (x.x0) j["x0"] = *x.x0;
        if (x.x1) j["x1"] = *x.x1;
        if (x.x2) j["x2"] = *x.x2;
        if (x.x3) j["x3"] = *x.x3;
        if (x.y0) j["y0"] = *x.y0;
        if (x.y1) j["y1"] = *x.y1;
        if (x.y2) j["y2"] = *x.y2;
        if (x.y3) j["y3"] = *x.y3;
        return j;
    }

    inline json to_partial_json(LanesConfig const & x) {
        json j = json::object();
        if (x.enabled) j["enabled"] = *x.enabled;
        if (x.region0) j["region0"] = to_partial_json(*x.region0);
        if (x.region1) j["region1"] = to_partial_json(*x.region1);
        if (x.region2) j["region2"] = to_partial_json(*x.region2);
        return j;
    }

    inline json to_partial_json(IoConfig const & x) {
        json j = json::object();
        if (x.can_flash) j["canFlash"] = *x.can_flash;
        if (x.can_trigger) j["canTrigger"] = *x.can_trigger;
        if (x.early_us) j["earlyUs"] = *x.early_us;
        if (x.group) j["group"] = *x.group;
        if (x.identifier) j["identifier"] = *x.identifier;
        if (x.is_input) j["isInput"] = *x.is_input;
        if (x.is_on) j["isOn"] = *x.is_on;
        j["port"] = x.port;
        if (x.protection) j["protection"] = *x.protection;
        if (x.type) j["type"] = *x.type;
        return j;
    }

    inline json to_partial_json(IoBasic const & x) {
        json j = json::object();
        if (x.is_input) j["isInput"] = *x.is_input;
        if (x.is_on) j["isOn"] = *x.is_on;
        j["port"] = x.port;
        return j;
    }

    inline json to_partial_json(Part const & x) {
        json j = json::object();
        j["content"] = x.content;
        j["name"] = x.name;
        j["type"] = x.type;
        return j;
    }

    inline json to_partial_json(Body const & x) {
        json j = json::object();
        j["parts"] = x.parts;
        j["variant"] = x.variant;
        return j;
    }

    inline json to_partial_json(Header const & x) {
        json j = json::object();
        j["name"] = x.name;
        j["value"] = x.value;
        return j;
    }

    inline json to_partial_json(Resolution const & x) {
        json j = json::object();
        j["height"] = x.height;
        j["width"] = x.width;
        return j;
    }

    inline json to_partial_json(Jpeg const & x) {
        json j = json::object();
        j["quality"] = x.quality;
        j["resolution"] = to_partial_json(x.resolution);
        return j;
    }

    inline json to_partial_json(Persistency const & x) {
        json j = json::object();
        j["enabled"] = x.enabled;
        j["maxDiskUsage"] = x.max_disk_usage;
        j["maxFileAge"] = x.max_file_age;
        j["newestFirst"] = x.newest_first;
        return j;
    }

    inline json to_partial_json(Url const & x) {
        json j = json::object();
        j["host"] = x.host;
        j["path"] = x.path;
        j["query"] = x.query;
        j["scheme"] = x.scheme;
        return j;
    }

    inline json to_partial_json(RestApiClientConfig const & x) {
        json j = json::object();
        j["body"] = to_partial_json(x.body);
        j["enabled"] = x.enabled;
        j["headers"] = x.headers;
        j["jpeg"] = to_partial_json(x.jpeg);
        j["method"] = x.method;
        j["persistency"] = to_partial_json(x.persistency);
        j["retries"] = x.retries;
        j["sendIndividualRequests"] = x.send_individual_requests;
        j["sendWithoutOcr"] = x.send_without_ocr;
        j["timeout"] = x.timeout;
        j["url"] = to_partial_json(x.url);
        return j;
    }

    inline json to_partial_json(RestApiClientStatus const & x) {
        json j = json::object();
        j["code"] = x.code;
        j["diskUsage"] = x.disk_usage;
        j["fileCount"] = x.file_count;
        j["message"] = x.message;
        return j;
    }

    inline json to_partial_json(AnalyticsClassifier const & x) {
        json j = json::object();
        if (x.customer) j["customer"] = *x.customer;
        if (x.max_connections) j["maxConnections"] = *x.max_connections;
        if (x.max_threads) j["maxThreads"] = *x.max_threads;
        if (x.serial) j["serial"] = *x.serial;
        if (x.sha1) j["sha1"] = *x.sha1;
        if (x.state) j["state"] = *x.state;
        if (x.ttl) j["ttl"] = *x.ttl;
        if (x.version) j["version"] = *x.version;
        return j;
    }

    inline json to_partial_json(AnalyticsOcr const & x) {
        json j = json::object();
        if (x.customer) j["customer"] = *x.customer;
        if (x.max_connections) j["maxConnections"] = *x.max_connections;
        if (x.max_threads) j["maxThreads"] = *x.max_threads;
        if (x.serial) j["serial"] = *x.serial;
        if (x.sha1) j["sha1"] = *x.sha1;
        if (x.state) j["state"] = *x.state;
        if (x.ttl) j["ttl"] = *x.ttl;
        if (x.version) j["version"] = *x.version;
        return j;
    }

    inline json to_partial_json(Analytics const & x) {
        json j = json::object();
        if (x.classifier) j["classifier"] = to_partial_json(*x.classifier);
        if (x.ocr) j["ocr"] = to_partial_json(*x.ocr);
        return j;
    }

    inline json to_partial_json(DeviceId const & x) {
        json j = json::object();
        if (x.serial) j["serial"] = *x.serial;
        return j;
    }

    inline json to_partial_json(Licenses const & x) {
        json j = json::object();
        if (x.analytics) j["analytics"] = to_partial_json(*x.analytics);
        if (x.device_id) j["deviceId"] = to_partial_json(*x.device_id);
        return j;
    }

}
}

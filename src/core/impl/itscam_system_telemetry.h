/*
 *  itscam_system_telemetry.h
 *
 *  ITSCAM Client SDK - recognizes the system telemetry document
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  A firmware without the system telemetry doesn't answer it with an error:
 *  the web interface returns its page to the GET, and pushes only its own
 *  status over the WebSocket.
 */

#ifndef ITSCAM_SYSTEM_TELEMETRY_H
#define ITSCAM_SYSTEM_TELEMETRY_H

#include "../itscam_types.h"
#include "../3rdparty/nlohmann/json.hpp"

#include <string>
#include <utility>

namespace itscam {
namespace detail {

inline bool isSystemTelemetry(const nlohmann::json& document) {
    return document.is_object() && document.contains("version");
}

/// Fails, as a 404 would, when a successful GET returned something else.
inline Result<nlohmann::json> requireSystemTelemetry(
    Result<nlohmann::json>&& response, const std::string& path) {
    if (response && !isSystemTelemetry(response.value())) {
        return Error{Error::InvalidParameter,
                     "GET " + path + ": the camera sent no telemetry "
                     "(firmware without this endpoint?)"};
    }
    return std::move(response);
}

}  // namespace detail
}  // namespace itscam

#endif  // ITSCAM_SYSTEM_TELEMETRY_H

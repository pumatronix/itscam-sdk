/*
 *  itscam_c_internal.h
 *
 *  ITSCAM Client SDK - Internal definitions shared by C API translation
 *  units.  Not installed; included from itscam_*_client_c.cpp only.
 *
 *  Copyright (c) 2026 Pumatronix
 */
#pragma once

#include <new>
#include <string>

struct ITSCAM_String {
    std::string data;
};

namespace itscam {
namespace c_internal {

inline ITSCAM_String* makeString(std::string s) {
    auto* out = new (std::nothrow) ITSCAM_String();
    if (!out) return nullptr;
    out->data = std::move(s);
    return out;
}

/// Set the thread-local error buffer surfaced by ITSCAM_getLastError().
/// Implemented in itscam_sdk_c.cpp so that all C translation units share a
/// single buffer.
void setLastError(const char* fmt, ...);

/// Convenience overload that copies a std::string into the thread-local
/// buffer.  Always safe to call with `err.message` from itscam::Error.
inline void setLastError(const std::string& message) {
    setLastError("%s", message.c_str());
}

}  // namespace c_internal
}  // namespace itscam

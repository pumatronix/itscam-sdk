/*
 *  itscam_sdk.h
 *
 *  ITSCAM Client SDK - Umbrella header
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Include this single header to get the full SDK surface.
 */

/**
 * @mainpage ITSCAM Client SDK -- C and C++ API reference
 *
 * This reference is generated from the public headers under
 * `src/core/`. For tutorials, integration guides, and the wrapper
 * documentation (Python, Go, C# / .NET), see the
 * [main documentation site](/itscam-sdk/).
 *
 * ## Public surface
 *
 * - `itscam::ItscamClient` -- binary TCP (Cougar) client on port 60000.
 * - `itscam::ItscamRestClient` -- HTTP/HTTPS JSON client.
 * - `itscam::ItscamCgiClient` -- CGI / multipart client.
 * - `itscam::Result<T>` and `itscam::Future<T>` -- shared error and
 *   async return types.
 *
 * ## Sub-APIs
 *
 * - @ref c_api -- C ABI consumed by the language wrappers.
 *
 * ## Single-header include
 *
 * `#include "itscam_sdk.h"` brings in every public C++ class, type,
 * and helper.
 */
#pragma once

#if defined(__has_include) && __has_include("itscam_sdk_version.h")
#include "itscam_sdk_version.h"
#else
/*
 * Fallback version macros for source-tree includes when the generated
 * itscam_sdk_version.h is not present yet (e.g. clean worktrees).
 */
#ifndef ITSCAM_SDK_VERSION_MAJOR
#define ITSCAM_SDK_VERSION_MAJOR 0
#endif
#ifndef ITSCAM_SDK_VERSION_MINOR
#define ITSCAM_SDK_VERSION_MINOR 0
#endif
#ifndef ITSCAM_SDK_VERSION_PATCH
#define ITSCAM_SDK_VERSION_PATCH 0
#endif
#ifndef ITSCAM_SDK_VERSION_STRING
#define ITSCAM_SDK_VERSION_STRING "0.0.0"
#endif
#ifndef ITSCAM_SDK_VERSION_FULL
#define ITSCAM_SDK_VERSION_FULL "0.0.0+unknown"
#endif
#ifndef ITSCAM_SDK_GIT_SHA
#define ITSCAM_SDK_GIT_SHA "0000000000000000000000000000000000000000"
#endif
#ifndef ITSCAM_SDK_GIT_SHA_SHORT
#define ITSCAM_SDK_GIT_SHA_SHORT "0000000"
#endif
#ifndef ITSCAM_SDK_BUILD_DATE
#define ITSCAM_SDK_BUILD_DATE "1970-01-01T00:00:00Z"
#endif
#endif

#include "itscam_types.h"
#include "itscam_client.h"
#include "itscam_rest_client.h"
#include "itscam_cgi_client.h"
#include "itscam_jpeg_utils.h"

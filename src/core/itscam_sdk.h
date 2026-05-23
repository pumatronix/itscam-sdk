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

#include "itscam_sdk_version.h"

#include "itscam_types.h"
#include "itscam_client.h"
#include "itscam_rest_client.h"
#include "itscam_cgi_client.h"
#include "itscam_jpeg_utils.h"

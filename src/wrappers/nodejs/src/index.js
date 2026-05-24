/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * Public entry point for the @pumatronix/itscam-sdk Node.js wrapper.
 */
'use strict';

const native = require('./native');
const errors = require('./errors');
const { ItscamClient, ConnectionState, LogLevel } = require('./itscam-client');
const { ItscamRestClient } = require('./itscam-rest-client');
const { ItscamCgiClient } = require('./itscam-cgi-client');
const jpegUtils = require('./jpeg-utils');
const versionInfo = require('./version');

/** Return the native libitscam_sdk version string (from `ITSCAM_getVersion`). */
function getNativeLibraryVersion() {
    return native.fns.getVersion() || '';
}

/** npm package version of this Node.js wrapper. */
function getWrapperVersion() {
    return versionInfo.VERSION;
}

/** Full version descriptor including git SHA and build date. */
function getWrapperVersionFull() {
    return versionInfo.VERSION_FULL;
}

/** Path on disk of the native shared library that was loaded. */
function getLibraryPath() {
    return native.libraryPath;
}

function getSystemLocalTime() {
    return native.fns.getSystemLocalTime();
}

function getSystemUtcTime() {
    return native.fns.getSystemUtcTime();
}

function getEpochTime() {
    return Number(native.fns.getEpochTime());
}

function getEpochTimeMs() {
    return Number(native.fns.getEpochTimeMs());
}

function getLastError() {
    return native.fns.getLastError() || '';
}

module.exports = {
    // Clients
    ItscamClient,
    ItscamRestClient,
    ItscamCgiClient,

    // Errors
    ItscamError: errors.ItscamError,
    ItscamTimeoutError: errors.ItscamTimeoutError,
    ItscamAuthError: errors.ItscamAuthError,
    ItscamConnectionError: errors.ItscamConnectionError,
    ItscamInvalidParameterError: errors.ItscamInvalidParameterError,
    ItscamServerError: errors.ItscamServerError,
    ErrorCode: errors.ErrorCode,

    // Enumerations
    ConnectionState,
    LogLevel,

    // JPEG metadata helpers (pure JS)
    extractJpegComment: jpegUtils.extractJpegComment,
    parseJpegCommentTags: jpegUtils.parseJpegCommentTags,
    extractPlateRecognitions: jpegUtils.extractPlateRecognitions,
    extractObjectDetections: jpegUtils.extractObjectDetections,
    parseJpegMetadata: jpegUtils.parseJpegMetadata,

    // System utilities
    getNativeLibraryVersion,
    getWrapperVersion,
    getWrapperVersionFull,
    getLibraryPath,
    getSystemLocalTime,
    getSystemUtcTime,
    getEpochTime,
    getEpochTimeMs,
    getLastError,
    VERSION: versionInfo.VERSION,
    VERSION_FULL: versionInfo.VERSION_FULL,
};

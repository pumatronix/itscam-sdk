/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * Error types for the ITSCAM SDK Node.js wrapper.
 */
'use strict';

const ErrorCode = Object.freeze({
    OK: 0,
    CONNECTION_FAILED: 1,
    TIMEOUT: 2,
    NOT_AUTHENTICATED: 3,
    INVALID_PARAMETER: 4,
    SERVER_ERROR: 5,
    DISCONNECTED: 6,
    UNKNOWN: 7,
    NULL_HANDLE: 8,
    ALLOCATION_FAILED: 9,
});

const ErrorCodeName = Object.freeze({
    0: 'OK',
    1: 'CONNECTION_FAILED',
    2: 'TIMEOUT',
    3: 'NOT_AUTHENTICATED',
    4: 'INVALID_PARAMETER',
    5: 'SERVER_ERROR',
    6: 'DISCONNECTED',
    7: 'UNKNOWN',
    8: 'NULL_HANDLE',
    9: 'ALLOCATION_FAILED',
});

class ItscamError extends Error {
    constructor(code, message) {
        const name = ErrorCodeName[code] || 'UNKNOWN';
        super(message
            ? '[' + name + '] ' + message
            : '[' + name + ']');
        this.name = 'ItscamError';
        this.code = code;
        this.codeName = name;
    }
}

class ItscamTimeoutError extends ItscamError {
    constructor(message) {
        super(ErrorCode.TIMEOUT, message);
        this.name = 'ItscamTimeoutError';
    }
}

class ItscamAuthError extends ItscamError {
    constructor(message) {
        super(ErrorCode.NOT_AUTHENTICATED, message);
        this.name = 'ItscamAuthError';
    }
}

class ItscamConnectionError extends ItscamError {
    constructor(code, message) {
        super(code, message);
        this.name = 'ItscamConnectionError';
    }
}

class ItscamInvalidParameterError extends ItscamError {
    constructor(message) {
        super(ErrorCode.INVALID_PARAMETER, message);
        this.name = 'ItscamInvalidParameterError';
    }
}

class ItscamServerError extends ItscamError {
    constructor(message) {
        super(ErrorCode.SERVER_ERROR, message);
        this.name = 'ItscamServerError';
    }
}

function throwIfFailed(rc, context, lastError) {
    if (rc === 0) return;

    const parts = [];
    if (context) parts.push(context);
    if (lastError) parts.push(lastError);
    const message = parts.join(': ');

    switch (rc) {
        case ErrorCode.TIMEOUT:
            throw new ItscamTimeoutError(message);
        case ErrorCode.NOT_AUTHENTICATED:
            throw new ItscamAuthError(message);
        case ErrorCode.CONNECTION_FAILED:
        case ErrorCode.DISCONNECTED:
            throw new ItscamConnectionError(rc, message);
        case ErrorCode.INVALID_PARAMETER:
            throw new ItscamInvalidParameterError(message);
        case ErrorCode.SERVER_ERROR:
            throw new ItscamServerError(message);
        default:
            throw new ItscamError(rc, message);
    }
}

module.exports = {
    ErrorCode,
    ErrorCodeName,
    ItscamError,
    ItscamTimeoutError,
    ItscamAuthError,
    ItscamConnectionError,
    ItscamInvalidParameterError,
    ItscamServerError,
    throwIfFailed,
};

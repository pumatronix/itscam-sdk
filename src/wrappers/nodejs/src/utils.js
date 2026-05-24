/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * Internal helpers shared by the three client modules.
 */
'use strict';

const { koffi, fns } = require('./native');
const { throwIfFailed } = require('./errors');

/**
 * Read an `ITSCAM_String *` (UTF-8 NUL-terminated) and free the
 * underlying handle.  Returns "" when the handle is NULL.
 */
function takeString(stringHandle) {
    if (!stringHandle) return '';
    try {
        const cstr = fns.String_data(stringHandle);
        return cstr ? koffi.decode(cstr, 'string') : '';
    } finally {
        fns.String_destroy(stringHandle);
    }
}

/** Read N bytes from a `uint8 *` into a fresh Node Buffer. */
function readBytes(ptr, size) {
    if (!ptr || size <= 0) return Buffer.alloc(0);
    return Buffer.from(koffi.decode(ptr, koffi.array('uint8', Number(size))));
}

/** Wrap an int-returning C call with the standard error-mapping policy. */
function checkCall(rc, context) {
    if (rc === 0) return;
    let last;
    try {
        last = fns.getLastError();
    } catch (_) {
        last = '';
    }
    throwIfFailed(rc, context, last || '');
}

module.exports = {
    takeString,
    readBytes,
    checkCall,
};

/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

import com.pumatronix.itscam.internal.NativeLibrary;

/**
 * Base exception for ITSCAM SDK errors.  The {@link ErrorCode} carries the
 * machine-readable failure mode (TIMEOUT, NOT_AUTHENTICATED, ...) for
 * callers that want to discriminate without parsing the message.
 */
public class ItscamException extends RuntimeException {

    private static final long serialVersionUID = 1L;

    private final ErrorCode code;

    public ItscamException(ErrorCode code, String message) {
        super(formatMessage(code, message));
        this.code = code;
    }

    public ItscamException(ErrorCode code, String message, Throwable cause) {
        super(formatMessage(code, message), cause);
        this.code = code;
    }

    public ErrorCode getCode() { return code; }

    private static String formatMessage(ErrorCode code, String message) {
        if (message == null || message.isEmpty()) {
            return "[" + code.name() + "]";
        }
        return "[" + code.name() + "] " + message;
    }

    /**
     * Throw a typed exception when {@code rc} is non-zero, picking the
     * narrowest subclass for the error category.  The native
     * {@code ITSCAM_getLastError()} message is appended when available.
     */
    public static void throwIfFailed(int rc, String context) {
        if (rc == 0) return;
        ErrorCode code = ErrorCode.fromInt(rc);
        String last = null;
        try {
            last = NativeLibrary.get().ITSCAM_getLastError();
        } catch (Throwable ignored) {
            /* getLastError is best-effort; never let it shadow the
             * underlying failure. */
        }
        StringBuilder sb = new StringBuilder();
        if (context != null && !context.isEmpty()) sb.append(context);
        if (last != null && !last.isEmpty()) {
            if (sb.length() > 0) sb.append(": ");
            sb.append(last);
        }
        String message = sb.toString();

        switch (code) {
            case TIMEOUT:
                throw new ItscamTimeoutException(message);
            case NOT_AUTHENTICATED:
                throw new ItscamAuthException(message);
            case CONNECTION_FAILED:
            case DISCONNECTED:
                throw new ItscamConnectionException(code, message);
            case INVALID_PARAMETER:
                throw new ItscamInvalidParameterException(message);
            case SERVER_ERROR:
                throw new ItscamServerException(message);
            default:
                throw new ItscamException(code, message);
        }
    }
}

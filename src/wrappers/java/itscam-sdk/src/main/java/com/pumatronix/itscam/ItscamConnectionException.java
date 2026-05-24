/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/** Connection-related failure (refused, dropped, TLS handshake, ...). */
public class ItscamConnectionException extends ItscamException {
    private static final long serialVersionUID = 1L;
    public ItscamConnectionException(ErrorCode code, String message) {
        super(code, message);
    }
}

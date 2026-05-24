/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/** Native call exceeded the configured timeout. */
public class ItscamTimeoutException extends ItscamException {
    private static final long serialVersionUID = 1L;
    public ItscamTimeoutException(String message) {
        super(ErrorCode.TIMEOUT, message);
    }
}

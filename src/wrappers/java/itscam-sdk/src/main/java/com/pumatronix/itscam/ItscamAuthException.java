/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/**
 * Authentication failed (HTTP 401 on REST, missing or expired token,
 * incorrect binary-client password).
 */
public class ItscamAuthException extends ItscamException {
    private static final long serialVersionUID = 1L;
    public ItscamAuthException(String message) {
        super(ErrorCode.NOT_AUTHENTICATED, message);
    }
}

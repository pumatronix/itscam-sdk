/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/** Server-side error (HTTP 5xx, internal SDK fault, ...). */
public class ItscamServerException extends ItscamException {
    private static final long serialVersionUID = 1L;
    public ItscamServerException(String message) {
        super(ErrorCode.SERVER_ERROR, message);
    }
}

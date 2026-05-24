/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam;

/** Native call rejected the supplied parameters. */
public class ItscamInvalidParameterException extends ItscamException {
    private static final long serialVersionUID = 1L;
    public ItscamInvalidParameterException(String message) {
        super(ErrorCode.INVALID_PARAMETER, message);
    }
}

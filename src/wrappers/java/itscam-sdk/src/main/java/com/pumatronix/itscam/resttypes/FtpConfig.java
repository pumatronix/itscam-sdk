/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.resttypes;

import com.google.gson.JsonObject;

/** FTP server configuration. */
public class FtpConfig extends RestObject {
    public FtpConfig() { super(); }
    public FtpConfig(String json) { super(json); }
    public FtpConfig(JsonObject json) { super(json); }
}
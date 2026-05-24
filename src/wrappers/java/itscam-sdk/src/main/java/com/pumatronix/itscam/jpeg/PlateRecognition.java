/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.jpeg;

/** Plate recognition parsed from the JPEG COM marker. */
public final class PlateRecognition {

    private final String plate;
    private final int    x;
    private final int    y;
    private final int    width;
    private final int    height;
    private final String color;
    private final int    countryCode;

    public PlateRecognition(String plate, int x, int y,
                            int width, int height,
                            String color, int countryCode) {
        this.plate = plate == null ? "" : plate;
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        this.color = color == null ? "" : color;
        this.countryCode = countryCode;
    }

    public String plate() { return plate; }
    public int    x() { return x; }
    public int    y() { return y; }
    public int    width() { return width; }
    public int    height() { return height; }
    public String color() { return color; }
    public int    countryCode() { return countryCode; }
}

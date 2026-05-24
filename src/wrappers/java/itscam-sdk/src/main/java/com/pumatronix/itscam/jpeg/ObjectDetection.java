/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 */
package com.pumatronix.itscam.jpeg;

/** Vehicle / object detection parsed from {@code ClassifierList} + {@code BMCList}. */
public final class ObjectDetection {

    private final int    type;
    private final double probability;
    private final int    x, y, width, height;
    private final String brand;
    private final double brandProb;
    private final String model;
    private final double modelProb;
    private final String color;
    private final double colorProb;

    public ObjectDetection(int type, double probability,
                           int x, int y, int width, int height,
                           String brand, double brandProb,
                           String model, double modelProb,
                           String color, double colorProb) {
        this.type = type;
        this.probability = probability;
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        this.brand = brand == null ? "" : brand;
        this.brandProb = brandProb;
        this.model = model == null ? "" : model;
        this.modelProb = modelProb;
        this.color = color == null ? "" : color;
        this.colorProb = colorProb;
    }

    public int    type() { return type; }
    public double probability() { return probability; }
    public int    x() { return x; }
    public int    y() { return y; }
    public int    width() { return width; }
    public int    height() { return height; }
    public String brand() { return brand; }
    public double brandProb() { return brandProb; }
    public String model() { return model; }
    public double modelProb() { return modelProb; }
    public String color() { return color; }
    public double colorProb() { return colorProb; }
}

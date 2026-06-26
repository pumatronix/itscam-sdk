/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * Pure-Java implementation of the JPEG COM marker parser used by the
 * ITSCAM camera.  Mirrors src/wrappers/python/itscam/jpeg_utils.py.
 */
package com.pumatronix.itscam.jpeg;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * One-shot extractor for the recognition / classification metadata that
 * ITSCAM cameras embed in the JPEG COM marker (0xFF 0xFE) using a
 * semicolon-delimited key=value format such as:
 *
 * <pre>
 * Placa=ABC1234;CoordPlaca=100x200,150x40;ClassifierList=[1,95,50,100,300,200]
 * </pre>
 */
public final class JpegMetadata {

    private final String comment;
    private final Map<String, String> tags;
    private final List<PlateRecognition> plates;
    private final List<ObjectDetection> objects;

    private JpegMetadata(String comment, Map<String, String> tags,
                         List<PlateRecognition> plates,
                         List<ObjectDetection> objects) {
        this.comment = comment;
        this.tags = Collections.unmodifiableMap(tags);
        this.plates = Collections.unmodifiableList(plates);
        this.objects = Collections.unmodifiableList(objects);
    }

    public String comment() { return comment; }
    public Map<String, String> tags() { return tags; }
    public List<PlateRecognition> plates() { return plates; }
    public List<ObjectDetection> objects() { return objects; }

    public static JpegMetadata empty() {
        return new JpegMetadata("", new LinkedHashMap<String, String>(),
            new ArrayList<PlateRecognition>(), new ArrayList<ObjectDetection>());
    }

    public static JpegMetadata parse(byte[] jpeg) {
        String c = extractComment(jpeg);
        if (c.isEmpty()) return empty();
        Map<String, String> t = parseTags(c);
        return new JpegMetadata(c, t,
                extractPlateRecognitions(t),
                extractObjectDetections(t));
    }

    /**
     * Locate and return the JPEG COM marker contents.  Scans backwards
     * from the EOI (0xFF 0xD9) like the camera firmware does.  Returns
     * an empty string when no comment is present.
     */
    public static String extractComment(byte[] data) {
        if (data == null || data.length < 4) return "";
        if ((data[data.length - 1] & 0xFF) != 0xD9) return "";
        for (int i = data.length - 1; i > 0; i--) {
            if ((data[i - 1] & 0xFF) == 0xFF
                    && (data[i] & 0xFF) == 0xFE) {
                int lenPos = i + 1;
                if (lenPos > data.length - 2) return "";
                int commentLen = ((data[lenPos] & 0xFF) << 8)
                        | (data[lenPos + 1] & 0xFF);
                if (commentLen < 2) return "";
                int bodyPos = lenPos + 2;
                int bodyLen = commentLen - 2;
                if (bodyPos + bodyLen > data.length) return "";
                return new String(data, bodyPos, bodyLen,
                        StandardCharsets.US_ASCII);
            }
        }
        return "";
    }

    /** Split a {@code Key=Value;Key=Value} string into a map. */
    public static Map<String, String> parseTags(String comment) {
        Map<String, String> out = new LinkedHashMap<>();
        if (comment == null || comment.isEmpty()) return out;
        for (String f : comment.split(";")) {
            int eq = f.indexOf('=');
            if (eq > 0) {
                out.put(f.substring(0, eq), f.substring(eq + 1));
            }
        }
        return out;
    }

    public static List<PlateRecognition> extractPlateRecognitions(
            Map<String, String> tags) {
        String placa = getOrDefault(tags, "Placa", "");
        if (placa.isEmpty()) return new ArrayList<>();

        String[] plates = placa.split("_");
        String[] coords = tags.containsKey("CoordPlaca")
                ? tags.get("CoordPlaca").split("_") : new String[0];
        String[] colors = tags.containsKey("CorPlaca")
                ? tags.get("CorPlaca").split("_") : new String[0];
        String[] cc = tags.containsKey("OCRCountryCode")
                ? tags.get("OCRCountryCode").split("_") : new String[0];

        List<PlateRecognition> out = new ArrayList<>();
        for (int i = 0; i < plates.length; i++) {
            if (plates[i].isEmpty()) continue;
            int x = 0, y = 0, w = 0, h = 0;
            if (i < coords.length && !coords[i].isEmpty()) {
                String[] parts = coords[i].split(",");
                if (parts.length >= 2) {
                    String[] xy = parts[0].split("x");
                    String[] wh = parts[1].split("x");
                    if (xy.length == 2) {
                        x = parseIntSafe(xy[0]);
                        y = parseIntSafe(xy[1]);
                    }
                    if (wh.length == 2) {
                        w = parseIntSafe(wh[0]);
                        h = parseIntSafe(wh[1]);
                    }
                }
            }
            String color = i < colors.length ? colors[i] : "";
            int country = i < cc.length ? parseIntSafe(cc[i]) : 0;
            out.add(new PlateRecognition(plates[i], x, y, w, h, color, country));
        }
        return out;
    }

    public static List<ObjectDetection> extractObjectDetections(
            Map<String, String> tags) {
        String classifierList = getOrDefault(tags, "ClassifierList", "");
        if (classifierList.isEmpty()) return new ArrayList<>();

        List<String> cls = parseBrackets(classifierList);
        List<String> bmc = parseBrackets(getOrDefault(tags, "BMCList", ""));

        List<ObjectDetection> out = new ArrayList<>();
        for (int i = 0; i < cls.size(); i++) {
            String[] parts = cls.get(i).split(",");
            if (parts.length < 6) continue;
            int type, prob, x, y, w, h;
            try {
                type = Integer.parseInt(parts[0].trim());
                prob = Integer.parseInt(parts[1].trim());
                x = Integer.parseInt(parts[2].trim());
                y = Integer.parseInt(parts[3].trim());
                w = Integer.parseInt(parts[4].trim());
                h = Integer.parseInt(parts[5].trim());
            } catch (NumberFormatException ex) {
                continue;
            }

            String brand = "", model = "", color = "";
            double brandP = 0, modelP = 0, colorP = 0;
            if (i < bmc.size()) {
                String cleaned = bmc.get(i).replace("\"", "");
                String[] bp = cleaned.split(",");
                if (bp.length >= 6) {
                    brand = bp[0];
                    brandP = parseIntSafe(bp[1]) / 100.0;
                    model = bp[2];
                    modelP = parseIntSafe(bp[3]) / 100.0;
                    color = bp[4];
                    colorP = parseIntSafe(bp[5]) / 100.0;
                }
            }
            out.add(new ObjectDetection(type, prob / 100.0, x, y, w, h,
                    brand, brandP, model, modelP, color, colorP));
        }
        return out;
    }

    private static List<String> parseBrackets(String raw) {
        List<String> out = new ArrayList<>();
        if (raw == null || raw.isEmpty()) return out;
        int pos = 0;
        while (pos < raw.length()) {
            int o = raw.indexOf('[', pos);
            if (o < 0) break;
            int c = raw.indexOf(']', o);
            if (c < 0) break;
            out.add(raw.substring(o + 1, c));
            pos = c + 1;
        }
        return out;
    }

    private static int parseIntSafe(String s) {
        if (s == null) return 0;
        String trimmed = s.trim();
        if (trimmed.isEmpty()) return 0;
        try {
            return Integer.parseInt(trimmed);
        } catch (NumberFormatException ex) {
            return 0;
        }
    }

    private static String getOrDefault(Map<String, String> map,
                                       String key, String defaultValue) {
        String value = map.get(key);
        return value == null ? defaultValue : value;
    }
}

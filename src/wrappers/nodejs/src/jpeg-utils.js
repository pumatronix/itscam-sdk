/*
 * SPDX-License-Identifier: Proprietary
 * Copyright (c) 2026 Pumatronix
 *
 * Pure-JavaScript implementation of the ITSCAM JPEG COM marker parser.
 * Mirrors src/wrappers/python/itscam/jpeg_utils.py.
 */
'use strict';

/** Extract the raw text of the JPEG COM marker (0xFF 0xFE), or "". */
function extractJpegComment(jpeg) {
    if (!jpeg || jpeg.length < 4) return '';
    const buf = Buffer.isBuffer(jpeg) ? jpeg : Buffer.from(jpeg);
    if (buf[buf.length - 1] !== 0xD9) return '';

    for (let i = buf.length - 1; i > 0; i--) {
        if (buf[i - 1] === 0xFF && buf[i] === 0xFE) {
            const lenPos = i + 1;
            if (lenPos > buf.length - 2) return '';
            const commentLen = (buf[lenPos] << 8) | buf[lenPos + 1];
            if (commentLen < 2) return '';
            const bodyPos = lenPos + 2;
            const bodyLen = commentLen - 2;
            if (bodyPos + bodyLen > buf.length) return '';
            return buf.slice(bodyPos, bodyPos + bodyLen)
                .toString('ascii');
        }
    }
    return '';
}

/** Split a `Key=Value;Key=Value` string into a plain object. */
function parseJpegCommentTags(comment) {
    const out = {};
    if (!comment) return out;
    for (const field of comment.split(';')) {
        const eq = field.indexOf('=');
        if (eq > 0) out[field.slice(0, eq)] = field.slice(eq + 1);
    }
    return out;
}

function _parseInt(s, fallback = 0) {
    if (s == null) return fallback;
    const n = parseInt(String(s).trim(), 10);
    return Number.isFinite(n) ? n : fallback;
}

function extractPlateRecognitions(tags) {
    const placa = tags && tags.Placa ? tags.Placa : '';
    if (!placa) return [];

    const plateTexts = placa.split('_');
    const coords = (tags.CoordPlaca || '').split('_');
    const colors = (tags.CorPlaca || '').split('_');
    const cc     = (tags.OCRCountryCode || '').split('_');

    const plates = [];
    for (let i = 0; i < plateTexts.length; i++) {
        const text = plateTexts[i];
        if (!text) continue;
        const pr = {
            plate: text, x: 0, y: 0, width: 0, height: 0,
            color: '', countryCode: 0,
        };
        if (i < coords.length && coords[i]) {
            const parts = coords[i].split(',');
            if (parts.length >= 2) {
                const xy = parts[0].split('x');
                const wh = parts[1].split('x');
                if (xy.length === 2) {
                    pr.x = _parseInt(xy[0]);
                    pr.y = _parseInt(xy[1]);
                }
                if (wh.length === 2) {
                    pr.width = _parseInt(wh[0]);
                    pr.height = _parseInt(wh[1]);
                }
            }
        }
        if (i < colors.length) pr.color = colors[i];
        if (i < cc.length) pr.countryCode = _parseInt(cc[i]);
        plates.push(pr);
    }
    return plates;
}

function _parseBrackets(raw) {
    const out = [];
    if (!raw) return out;
    let pos = 0;
    while (pos < raw.length) {
        const o = raw.indexOf('[', pos);
        if (o < 0) break;
        const c = raw.indexOf(']', o);
        if (c < 0) break;
        out.push(raw.slice(o + 1, c));
        pos = c + 1;
    }
    return out;
}

function extractObjectDetections(tags) {
    const cls = (tags && tags.ClassifierList) ? tags.ClassifierList : '';
    if (!cls) return [];

    const clsEntries = _parseBrackets(cls);
    const bmcEntries = _parseBrackets((tags && tags.BMCList) || '');

    const detections = [];
    for (let i = 0; i < clsEntries.length; i++) {
        const parts = clsEntries[i].split(',');
        if (parts.length < 6) continue;
        const det = {
            type: _parseInt(parts[0]),
            probability: _parseInt(parts[1]) / 100.0,
            x: _parseInt(parts[2]),
            y: _parseInt(parts[3]),
            width: _parseInt(parts[4]),
            height: _parseInt(parts[5]),
            brand: '', brandProb: 0,
            model: '', modelProb: 0,
            color: '', colorProb: 0,
        };
        if (i < bmcEntries.length) {
            const cleaned = bmcEntries[i].replace(/"/g, '');
            const bp = cleaned.split(',');
            if (bp.length >= 6) {
                det.brand = bp[0];
                det.brandProb = _parseInt(bp[1]) / 100.0;
                det.model = bp[2];
                det.modelProb = _parseInt(bp[3]) / 100.0;
                det.color = bp[4];
                det.colorProb = _parseInt(bp[5]) / 100.0;
            }
        }
        detections.push(det);
    }
    return detections;
}

function parseJpegMetadata(jpeg) {
    const comment = extractJpegComment(jpeg);
    if (!comment) {
        return { comment: '', tags: {}, plates: [], objects: [] };
    }
    const tags = parseJpegCommentTags(comment);
    return {
        comment,
        tags,
        plates: extractPlateRecognitions(tags),
        objects: extractObjectDetections(tags),
    };
}

module.exports = {
    extractJpegComment,
    parseJpegCommentTags,
    extractPlateRecognitions,
    extractObjectDetections,
    parseJpegMetadata,
};

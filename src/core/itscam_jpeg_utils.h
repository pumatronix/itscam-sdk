/*
 *  itscam_jpeg_utils.h
 *
 *  ITSCAM Client SDK - JPEG metadata extraction utilities
 *
 *  Copyright (c) 2026 Pumatronix
 *
 *  Self-contained header (no external dependencies beyond itscam_types.h).
 *
 *  Provides functions to extract and parse metadata from JPEG COM markers
 *  (0xFF 0xFE) as produced by ITSCAM cameras.  The comment body uses a
 *  semicolon-delimited key=value format:
 *
 *      "Placa=ABC1234;CoordPlaca=100x200,150x40;ClassifierList=[1,95,50,100,300,200];..."
 *
 *  Typical keys for plate recognition:
 *      Placa           Plate text (multiple plates separated by '_')
 *      CoordPlaca      Bounding box per plate: "XxY,WxH" (separated by '_')
 *      CorPlaca        Plate color per plate (separated by '_')
 *      OCRCountryCode  Country code per plate (separated by '_')
 *
 *  Typical keys for vehicle classification:
 *      ClassifierList  "[type,prob,x,y,w,h],[...]"  (prob 0-100)
 *      BMCList         "[\"brand\",bProb,\"model\",mProb,\"color\",cProb],[...]"
 */
#pragma once

#include "itscam_types.h"

#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace itscam {

//=========================================================================
// Low-level: extract the raw JPEG COM marker string
//=========================================================================

/**
 * @brief Extract the comment string from a JPEG's COM marker (0xFF 0xFE).
 *
 * Scans backwards from the end of the JPEG for the COM marker, then reads
 * the comment body.  Returns an empty string if no COM marker is found or
 * the JPEG is malformed.
 *
 * @param jpegData  Pointer to the raw JPEG bytes.
 * @param jpegSize  Size of the JPEG buffer in bytes.
 * @return The comment body as a string, or empty if not found.
 */
inline std::string extractJpegComment(const uint8_t* jpegData, size_t jpegSize)
{
    if (!jpegData) return {};
    if (jpegSize < 4) return {};

    // Verify valid JPEG ending (EOI marker: 0xFF 0xD9)
    if (jpegData[jpegSize - 1] != 0xD9) return {};

    // Scan backwards for COM marker (0xFF 0xFE)
    for (size_t i = jpegSize - 1; i > 0; --i) {
        if (jpegData[i - 1] == 0xFF && jpegData[i] == 0xFE) {
            // 2-byte big-endian length follows the marker
            size_t lenPos = i + 1;
            if (lenPos > jpegSize - 2) return {};
            uint16_t commentLen = static_cast<uint16_t>(
                (jpegData[lenPos] << 8) | jpegData[lenPos + 1]);

            if (commentLen < 2) return {};

            // The length field includes itself (2 bytes), so body starts at lenPos+2
            size_t bodyPos = lenPos + 2;
            if (bodyPos > jpegSize) return {};

            size_t bodyLen = static_cast<size_t>(commentLen - 2);
            if (bodyLen > jpegSize - bodyPos) return {};

            return std::string(reinterpret_cast<const char*>(jpegData + bodyPos),
                               bodyLen);
        }
    }
    return {};
}

/// @overload Convenience for std::vector<uint8_t>.
inline std::string extractJpegComment(const std::vector<uint8_t>& jpeg)
{
    if (jpeg.empty()) return {};
    return extractJpegComment(jpeg.data(), jpeg.size());
}

//=========================================================================
// Mid-level: parse comment string into a key=value tag map
//=========================================================================

namespace detail {

inline std::vector<std::string> splitChar(const std::string& s, char delim)
{
    std::vector<std::string> out;
    std::string token;
    for (char ch : s) {
        if (ch == delim) {
            out.push_back(std::move(token));
            token.clear();
        } else {
            token += ch;
        }
    }
    if (!token.empty()) out.push_back(std::move(token));
    return out;
}

} // namespace detail

/**
 * @brief Parse a JPEG comment string into a key=value map.
 *
 * The comment format is: "Key1=Value1;Key2=Value2;..."
 * Keys and values are NOT URL-encoded; they are plain ASCII strings.
 *
 * @param comment  Raw comment string (from extractJpegComment).
 * @return Map of tag name -> tag value.
 */
inline std::map<std::string, std::string> parseJpegCommentTags(
    const std::string& comment)
{
    std::map<std::string, std::string> tags;
    if (comment.empty()) return tags;

    auto fields = detail::splitChar(comment, ';');
    for (const auto& field : fields) {
        // Find the first '=' to split key and value
        auto eqPos = field.find('=');
        if (eqPos != std::string::npos && eqPos > 0) {
            tags[field.substr(0, eqPos)] = field.substr(eqPos + 1);
        }
    }
    return tags;
}

/// @overload Convenience: extract comment from JPEG bytes, then parse tags.
inline std::map<std::string, std::string> parseJpegCommentTags(
    const std::vector<uint8_t>& jpeg)
{
    return parseJpegCommentTags(extractJpegComment(jpeg));
}

//=========================================================================
// High-level: extract structured recognition / detection results
//=========================================================================

/**
 * @brief Extract plate recognition results from JPEG comment tags.
 *
 * Uses the following tags:
 *   - Placa:        Plate text(s), underscore-separated for multiple plates.
 *   - CoordPlaca:   Bounding boxes, underscore-separated.
 *                   Each box is "XxY,WxH" (e.g. "120x300,180x50").
 *   - CorPlaca:     Plate color code(s), underscore-separated ("0" or "1").
 *   - OCRCountryCode: Country code(s), underscore-separated.
 *
 * @param tags  Tag map from parseJpegCommentTags().
 * @return Vector of PlateRecognition results (may be empty).
 */
inline std::vector<PlateRecognition> extractPlateRecognitions(
    const std::map<std::string, std::string>& tags)
{
    std::vector<PlateRecognition> plates;

    auto itPlaca = tags.find("Placa");
    if (itPlaca == tags.end() || itPlaca->second.empty()) return plates;

    auto plateTexts = detail::splitChar(itPlaca->second, '_');

    // Optional coordinate strings
    std::vector<std::string> coordTexts;
    auto itCoord = tags.find("CoordPlaca");
    if (itCoord != tags.end() && !itCoord->second.empty()) {
        coordTexts = detail::splitChar(itCoord->second, '_');
    }

    // Optional color strings
    std::vector<std::string> colorTexts;
    auto itColor = tags.find("CorPlaca");
    if (itColor != tags.end() && !itColor->second.empty()) {
        colorTexts = detail::splitChar(itColor->second, '_');
    }

    // Optional country codes
    std::vector<std::string> countryTexts;
    auto itCountry = tags.find("OCRCountryCode");
    if (itCountry != tags.end() && !itCountry->second.empty()) {
        countryTexts = detail::splitChar(itCountry->second, '_');
    }

    for (size_t i = 0; i < plateTexts.size(); ++i) {
        if (plateTexts[i].empty()) continue;

        PlateRecognition pr;
        pr.plate = plateTexts[i];

        // Parse "XxY,WxH" coordinate string
        if (i < coordTexts.size()) {
            auto parts = detail::splitChar(coordTexts[i], ',');
            if (parts.size() >= 2) {
                auto xy = detail::splitChar(parts[0], 'x');
                auto wh = detail::splitChar(parts[1], 'x');
                if (xy.size() == 2) {
                    pr.x = std::atoi(xy[0].c_str());
                    pr.y = std::atoi(xy[1].c_str());
                }
                if (wh.size() == 2) {
                    pr.width  = std::atoi(wh[0].c_str());
                    pr.height = std::atoi(wh[1].c_str());
                }
            }
        }

        // Plate color ("0" = white on black, "1" = black on white)
        if (i < colorTexts.size()) {
            pr.color = colorTexts[i];
        }

        // Country code
        if (i < countryTexts.size()) {
            pr.countryCode = std::atoi(countryTexts[i].c_str());
        }

        plates.push_back(std::move(pr));
    }
    return plates;
}

/**
 * @brief Extract vehicle/object detection results from JPEG comment tags.
 *
 * Uses the following tags:
 *   - ClassifierList: "[type,prob,x,y,w,h],[type,prob,x,y,w,h],..."
 *                     prob is an integer 0-100 (percentage).
 *   - BMCList:        '["brand",bProb,"model",mProb,"color",cProb],...'
 *                     Probabilities are 0-100 integers.
 *
 * @param tags  Tag map from parseJpegCommentTags().
 * @return Vector of ObjectDetection results (may be empty).
 */
inline std::vector<ObjectDetection> extractObjectDetections(
    const std::map<std::string, std::string>& tags)
{
    std::vector<ObjectDetection> detections;

    auto itCls = tags.find("ClassifierList");
    if (itCls == tags.end() || itCls->second.empty()) return detections;

    // Parse each "[...]" block from ClassifierList
    const std::string& clsRaw = itCls->second;
    std::vector<std::string> clsEntries;
    {
        size_t pos = 0;
        while (pos < clsRaw.size()) {
            size_t open = clsRaw.find('[', pos);
            if (open == std::string::npos) break;
            size_t close = clsRaw.find(']', open);
            if (close == std::string::npos) break;
            clsEntries.push_back(clsRaw.substr(open + 1, close - open - 1));
            pos = close + 1;
        }
    }

    // Parse BMCList blocks if present
    std::vector<std::string> bmcEntries;
    auto itBmc = tags.find("BMCList");
    if (itBmc != tags.end() && !itBmc->second.empty()) {
        const std::string& bmcRaw = itBmc->second;
        size_t pos = 0;
        while (pos < bmcRaw.size()) {
            size_t open = bmcRaw.find('[', pos);
            if (open == std::string::npos) break;
            size_t close = bmcRaw.find(']', open);
            if (close == std::string::npos) break;
            bmcEntries.push_back(bmcRaw.substr(open + 1, close - open - 1));
            pos = close + 1;
        }
    }

    for (size_t i = 0; i < clsEntries.size(); ++i) {
        // Each entry: "type,prob,x,y,w,h"
        auto parts = detail::splitChar(clsEntries[i], ',');
        if (parts.size() < 6) continue;

        ObjectDetection od;
        od.type        = std::atoi(parts[0].c_str());
        od.probability = std::atoi(parts[1].c_str()) / 100.0f;
        od.x           = std::atoi(parts[2].c_str());
        od.y           = std::atoi(parts[3].c_str());
        od.width       = std::atoi(parts[4].c_str());
        od.height      = std::atoi(parts[5].c_str());

        // Merge BMC data if available
        // Raw format: "brand",bProb,"model",mProb,"color",cProb
        if (i < bmcEntries.size()) {
            // Strip double-quotes, then split on comma
            std::string cleaned;
            for (char ch : bmcEntries[i]) {
                if (ch != '"') cleaned += ch;
            }
            // Result: brand,bProb,model,mProb,color,cProb
            auto bmcParts = detail::splitChar(cleaned, ',');
            if (bmcParts.size() >= 6) {
                od.brand     = bmcParts[0];
                od.brandProb = std::atoi(bmcParts[1].c_str()) / 100.0f;
                od.model     = bmcParts[2];
                od.modelProb = std::atoi(bmcParts[3].c_str()) / 100.0f;
                od.color     = bmcParts[4];
                od.colorProb = std::atoi(bmcParts[5].c_str()) / 100.0f;
            }
        }

        detections.push_back(std::move(od));
    }
    return detections;
}

//=========================================================================
// Convenience: populate a CaptureResult from JPEG comment metadata
//=========================================================================

/**
 * @brief Populate plate and object detection fields in a CaptureResult
 *        by parsing the JPEG COM marker metadata.
 *
 * This is useful as a fallback when the protocol layer (IMGPKG mixed body)
 * does not include detection metadata (e.g. older firmware).
 *
 * Only populates fields if both info.plates and info.objects are currently
 * empty — i.e. it will not overwrite data already provided by the protocol.
 *
 * @param[in,out] result  CaptureResult whose jpeg vector will be parsed.
 *                        On return, info.plates and info.objects may be filled.
 * @return true if JPEG comment metadata was found and parsed, false otherwise.
 */
inline bool populateFromJpegComment(CaptureResult& result)
{
    // Only act as fallback — don't overwrite protocol-provided data
    if (!result.info.plates.empty() || !result.info.objects.empty()) {
        return false;
    }
    if (result.jpeg.empty()) return false;

    auto tags = parseJpegCommentTags(result.jpeg);
    if (tags.empty()) return false;

    result.info.plates  = extractPlateRecognitions(tags);
    result.info.objects = extractObjectDetections(tags);

    return !result.info.plates.empty() || !result.info.objects.empty();
}

} // namespace itscam

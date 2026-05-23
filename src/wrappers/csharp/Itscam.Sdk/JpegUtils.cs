// SPDX-License-Identifier: Proprietary
// Copyright (c) 2026 Pumatronix

using System;
using System.Collections.Generic;
using System.Text;

namespace Pumatronix.Itscam
{
    /// <summary>
    /// Utilities for extracting metadata from the JPEG COM marker (0xFF 0xFE)
    /// embedded by ITSCAM cameras.
    ///
    /// <para>
    /// The comment body uses a semicolon-delimited key=value format:
    /// <c>Placa=ABC1234;CoordPlaca=100x200,150x40;ClassifierList=[1,95,50,100,300,200]</c>
    /// </para>
    /// </summary>
    public static class JpegUtils
    {
        /// <summary>
        /// Extract the raw comment string from a JPEG's COM marker (0xFF 0xFE).
        /// Scans backwards from the end of the JPEG data for the COM marker.
        /// </summary>
        /// <param name="jpeg">Raw JPEG bytes.</param>
        /// <returns>The comment body, or <see cref="string.Empty"/> if not found.</returns>
        public static string ExtractComment(byte[] jpeg)
        {
            if (jpeg == null || jpeg.Length < 4)
                return string.Empty;

            if (jpeg[jpeg.Length - 1] != 0xD9)
                return string.Empty;

            for (int i = jpeg.Length - 1; i > 0; i--)
            {
                if (jpeg[i - 1] == 0xFF && jpeg[i] == 0xFE)
                {
                    int lenPos = i + 1;
                    if (lenPos > jpeg.Length - 2)
                        return string.Empty;

                    int commentLen = (jpeg[lenPos] << 8) | jpeg[lenPos + 1];
                    if (commentLen < 2)
                        return string.Empty;

                    int bodyPos = lenPos + 2;
                    if (bodyPos > jpeg.Length)
                        return string.Empty;

                    int bodyLen = commentLen - 2;
                    if (bodyLen > jpeg.Length - bodyPos)
                        return string.Empty;

                    return Encoding.ASCII.GetString(jpeg, bodyPos, bodyLen);
                }
            }

            return string.Empty;
        }

        /// <summary>
        /// Extract the raw comment string from a JPEG's COM marker (0xFF 0xFE).
        /// </summary>
        /// <param name="jpeg">Raw JPEG bytes.</param>
        /// <returns>The comment body, or <see cref="string.Empty"/> if not found.</returns>
        public static string ExtractComment(ReadOnlySpan<byte> jpeg)
        {
            if (jpeg.Length < 4)
                return string.Empty;

            if (jpeg[jpeg.Length - 1] != 0xD9)
                return string.Empty;

            for (int i = jpeg.Length - 1; i > 0; i--)
            {
                if (jpeg[i - 1] == 0xFF && jpeg[i] == 0xFE)
                {
                    int lenPos = i + 1;
                    if (lenPos > jpeg.Length - 2)
                        return string.Empty;

                    int commentLen = (jpeg[lenPos] << 8) | jpeg[lenPos + 1];
                    if (commentLen < 2)
                        return string.Empty;

                    int bodyPos = lenPos + 2;
                    if (bodyPos > jpeg.Length)
                        return string.Empty;

                    int bodyLen = commentLen - 2;
                    if (bodyLen > jpeg.Length - bodyPos)
                        return string.Empty;

                    return Encoding.ASCII.GetString(
                        jpeg.Slice(bodyPos, bodyLen).ToArray(), 0, bodyLen);
                }
            }

            return string.Empty;
        }

        /// <summary>
        /// Parse a JPEG comment string into a key=value dictionary.
        /// </summary>
        /// <param name="comment">Raw comment string.</param>
        /// <returns>Tag name to value mapping.</returns>
        public static IReadOnlyDictionary<string, string> ParseCommentTags(string comment)
        {
            var tags = new Dictionary<string, string>();
            if (string.IsNullOrEmpty(comment))
                return tags;

            foreach (var field in comment.Split(';'))
            {
                int eq = field.IndexOf('=');
                if (eq > 0)
                    tags[field.Substring(0, eq)] = field.Substring(eq + 1);
            }

            return tags;
        }

        /// <summary>
        /// One-shot convenience: extract the JPEG COM comment and parse it
        /// into a key=value tag dictionary.
        /// </summary>
        /// <param name="jpeg">Raw JPEG bytes.</param>
        /// <returns>Tag name to value mapping (empty if no COM marker).</returns>
        public static IReadOnlyDictionary<string, string> ParseCommentTags(byte[] jpeg)
        {
            return ParseCommentTags(ExtractComment(jpeg));
        }

        /// <summary>
        /// Extract structured plate-recognition results from parsed comment tags.
        /// </summary>
        public static IReadOnlyList<PlateRecognitionResult> ExtractPlateRecognitions(
            IReadOnlyDictionary<string, string> tags)
        {
            var plates = new List<PlateRecognitionResult>();
            if (!tags.TryGetValue("Placa", out var placa) || string.IsNullOrEmpty(placa))
                return plates;

            var plateTexts = placa.Split('_');
            var coordTexts = tags.TryGetValue("CoordPlaca", out var c) && !string.IsNullOrEmpty(c)
                ? c.Split('_') : Array.Empty<string>();
            var colorTexts = tags.TryGetValue("CorPlaca", out var col) && !string.IsNullOrEmpty(col)
                ? col.Split('_') : Array.Empty<string>();
            var countryTexts = tags.TryGetValue("OCRCountryCode", out var cc) && !string.IsNullOrEmpty(cc)
                ? cc.Split('_') : Array.Empty<string>();

            for (int i = 0; i < plateTexts.Length; i++)
            {
                if (string.IsNullOrEmpty(plateTexts[i]))
                    continue;

                var pr = new PlateRecognitionResult { Plate = plateTexts[i] };

                if (i < coordTexts.Length && !string.IsNullOrEmpty(coordTexts[i]))
                {
                    var parts = coordTexts[i].Split(',');
                    if (parts.Length >= 2)
                    {
                        var xy = parts[0].Split('x');
                        var wh = parts[1].Split('x');
                        if (xy.Length == 2)
                        {
                            int.TryParse(xy[0], out var px);
                            int.TryParse(xy[1], out var py);
                            pr.X = px; pr.Y = py;
                        }
                        if (wh.Length == 2)
                        {
                            int.TryParse(wh[0], out var pw);
                            int.TryParse(wh[1], out var ph);
                            pr.Width = pw; pr.Height = ph;
                        }
                    }
                }

                if (i < colorTexts.Length)
                    pr.Color = colorTexts[i];
                if (i < countryTexts.Length && int.TryParse(countryTexts[i], out var ccode))
                    pr.CountryCode = ccode;

                plates.Add(pr);
            }

            return plates;
        }

        /// <summary>
        /// Extract structured vehicle/object detection results from parsed
        /// comment tags.
        /// </summary>
        public static IReadOnlyList<ObjectDetectionResult> ExtractObjectDetections(
            IReadOnlyDictionary<string, string> tags)
        {
            var detections = new List<ObjectDetectionResult>();
            if (!tags.TryGetValue("ClassifierList", out var clsRaw) || string.IsNullOrEmpty(clsRaw))
                return detections;

            var clsEntries = ParseBracketEntries(clsRaw);
            var bmcEntries = tags.TryGetValue("BMCList", out var bmcRaw)
                ? ParseBracketEntries(bmcRaw)
                : new List<string>();

            for (int i = 0; i < clsEntries.Count; i++)
            {
                var parts = clsEntries[i].Split(',');
                if (parts.Length < 6) continue;

                if (!int.TryParse(parts[0], out var type) ||
                    !int.TryParse(parts[1], out var prob) ||
                    !int.TryParse(parts[2], out var x) ||
                    !int.TryParse(parts[3], out var y) ||
                    !int.TryParse(parts[4], out var w) ||
                    !int.TryParse(parts[5], out var h))
                    continue;

                var od = new ObjectDetectionResult
                {
                    Type = type,
                    Probability = prob / 100f,
                    X = x, Y = y, Width = w, Height = h
                };

                if (i < bmcEntries.Count)
                {
                    var cleaned = bmcEntries[i].Replace("\"", "");
                    var bmc = cleaned.Split(',');
                    if (bmc.Length >= 6)
                    {
                        od.Brand = bmc[0];
                        if (int.TryParse(bmc[1], out var bp)) od.BrandProb = bp / 100f;
                        od.Model = bmc[2];
                        if (int.TryParse(bmc[3], out var mp)) od.ModelProb = mp / 100f;
                        od.Color = bmc[4];
                        if (int.TryParse(bmc[5], out var cp)) od.ColorProb = cp / 100f;
                    }
                }

                detections.Add(od);
            }

            return detections;
        }

        private static List<string> ParseBracketEntries(string raw)
        {
            var entries = new List<string>();
            int pos = 0;
            while (pos < raw.Length)
            {
                int open = raw.IndexOf('[', pos);
                if (open < 0) break;
                int close = raw.IndexOf(']', open);
                if (close < 0) break;
                entries.Add(raw.Substring(open + 1, close - open - 1));
                pos = close + 1;
            }
            return entries;
        }
    }

    /// <summary>Plate recognition result parsed from JPEG COM metadata.</summary>
    public sealed class PlateRecognitionResult
    {
        public string Plate { get; set; } = string.Empty;
        public int X { get; set; }
        public int Y { get; set; }
        public int Width { get; set; }
        public int Height { get; set; }
        public string Color { get; set; } = string.Empty;
        public int CountryCode { get; set; }
    }

    /// <summary>Vehicle/object detection result parsed from JPEG COM metadata.</summary>
    public sealed class ObjectDetectionResult
    {
        public int Type { get; set; }
        public float Probability { get; set; }
        public int X { get; set; }
        public int Y { get; set; }
        public int Width { get; set; }
        public int Height { get; set; }
        public string Brand { get; set; } = string.Empty;
        public float BrandProb { get; set; }
        public string Model { get; set; } = string.Empty;
        public float ModelProb { get; set; }
        public string Color { get; set; } = string.Empty;
        public float ColorProb { get; set; }
    }
}

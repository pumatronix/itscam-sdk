package itscam

import "strings"

// ExtractJpegComment extracts the comment string from a JPEG's COM marker
// (0xFF 0xFE).  ITSCAM cameras embed recognition and classification metadata
// in this marker using a semicolon-delimited key=value format such as:
//
//	Placa=ABC1234;CoordPlaca=100x200,150x40;ClassifierList=[1,95,50,100,300,200]
//
// Returns an empty string if the JPEG has no COM marker or is malformed.
func ExtractJpegComment(jpeg []byte) string {
	size := len(jpeg)
	if size < 4 {
		return ""
	}
	if jpeg[size-1] != 0xD9 {
		return ""
	}

	for i := size - 1; i > 0; i-- {
		if jpeg[i-1] == 0xFF && jpeg[i] == 0xFE {
			lenPos := i + 1
			if lenPos > size-2 {
				return ""
			}
			commentLen := int(jpeg[lenPos])<<8 | int(jpeg[lenPos+1])
			if commentLen < 2 {
				return ""
			}
			bodyPos := lenPos + 2
			if bodyPos > size {
				return ""
			}
			bodyLen := commentLen - 2
			if bodyLen > size-bodyPos {
				return ""
			}
			return string(jpeg[bodyPos : bodyPos+bodyLen])
		}
	}
	return ""
}

// ParseJpegCommentTags parses a JPEG comment string (from
// ExtractJpegComment) into a key=value map.
//
// The expected format is "Key1=Value1;Key2=Value2;...".
func ParseJpegCommentTags(comment string) map[string]string {
	tags := make(map[string]string)
	if comment == "" {
		return tags
	}
	for _, field := range strings.Split(comment, ";") {
		eq := strings.IndexByte(field, '=')
		if eq > 0 {
			tags[field[:eq]] = field[eq+1:]
		}
	}
	return tags
}

// PlateRecognitionResult holds a single plate recognition parsed from JPEG
// COM metadata.
type PlateRecognitionResult struct {
	Plate       string
	X, Y        int
	Width       int
	Height      int
	Color       string
	CountryCode int
}

// ObjectDetectionResult holds a single vehicle/object detection parsed from
// JPEG COM metadata.
type ObjectDetectionResult struct {
	Type        int
	Probability float32
	X, Y        int
	Width       int
	Height      int
	Brand       string
	BrandProb   float32
	Model       string
	ModelProb   float32
	Color       string
	ColorProb   float32
}

// ExtractPlateRecognitions extracts structured plate-recognition results from
// parsed comment tags.
func ExtractPlateRecognitions(tags map[string]string) []PlateRecognitionResult {
	placa, ok := tags["Placa"]
	if !ok || placa == "" {
		return nil
	}

	plateTexts := strings.Split(placa, "_")

	var coordTexts, colorTexts, countryTexts []string
	if v, ok := tags["CoordPlaca"]; ok && v != "" {
		coordTexts = strings.Split(v, "_")
	}
	if v, ok := tags["CorPlaca"]; ok && v != "" {
		colorTexts = strings.Split(v, "_")
	}
	if v, ok := tags["OCRCountryCode"]; ok && v != "" {
		countryTexts = strings.Split(v, "_")
	}

	var plates []PlateRecognitionResult
	for i, text := range plateTexts {
		if text == "" {
			continue
		}
		pr := PlateRecognitionResult{Plate: text}

		if i < len(coordTexts) && coordTexts[i] != "" {
			parts := strings.Split(coordTexts[i], ",")
			if len(parts) >= 2 {
				xy := strings.Split(parts[0], "x")
				wh := strings.Split(parts[1], "x")
				if len(xy) == 2 {
					pr.X = atoi(xy[0])
					pr.Y = atoi(xy[1])
				}
				if len(wh) == 2 {
					pr.Width = atoi(wh[0])
					pr.Height = atoi(wh[1])
				}
			}
		}

		if i < len(colorTexts) {
			pr.Color = colorTexts[i]
		}
		if i < len(countryTexts) {
			pr.CountryCode = atoi(countryTexts[i])
		}

		plates = append(plates, pr)
	}
	return plates
}

// ExtractObjectDetections extracts structured vehicle/object detection
// results from parsed comment tags.
func ExtractObjectDetections(tags map[string]string) []ObjectDetectionResult {
	clsRaw, ok := tags["ClassifierList"]
	if !ok || clsRaw == "" {
		return nil
	}

	clsEntries := parseBrackets(clsRaw)
	var bmcEntries []string
	if bmcRaw, ok := tags["BMCList"]; ok {
		bmcEntries = parseBrackets(bmcRaw)
	}

	var detections []ObjectDetectionResult
	for i, entry := range clsEntries {
		parts := strings.Split(entry, ",")
		if len(parts) < 6 {
			continue
		}

		od := ObjectDetectionResult{
			Type:        atoi(parts[0]),
			Probability: float32(atoi(parts[1])) / 100.0,
			X:           atoi(parts[2]),
			Y:           atoi(parts[3]),
			Width:       atoi(parts[4]),
			Height:      atoi(parts[5]),
		}

		if i < len(bmcEntries) {
			cleaned := strings.ReplaceAll(bmcEntries[i], "\"", "")
			bmc := strings.Split(cleaned, ",")
			if len(bmc) >= 6 {
				od.Brand = bmc[0]
				od.BrandProb = float32(atoi(bmc[1])) / 100.0
				od.Model = bmc[2]
				od.ModelProb = float32(atoi(bmc[3])) / 100.0
				od.Color = bmc[4]
				od.ColorProb = float32(atoi(bmc[5])) / 100.0
			}
		}

		detections = append(detections, od)
	}
	return detections
}

// JpegCommentMetadata groups all metadata extracted from a JPEG COM marker.
type JpegCommentMetadata struct {
	Comment string
	Tags    map[string]string
	Plates  []PlateRecognitionResult
	Objects []ObjectDetectionResult
}

// ParseJpegMetadata performs one-shot extraction and parsing of all JPEG COM
// marker metadata.
func ParseJpegMetadata(jpeg []byte) JpegCommentMetadata {
	comment := ExtractJpegComment(jpeg)
	if comment == "" {
		return JpegCommentMetadata{Tags: make(map[string]string)}
	}
	tags := ParseJpegCommentTags(comment)
	return JpegCommentMetadata{
		Comment: comment,
		Tags:    tags,
		Plates:  ExtractPlateRecognitions(tags),
		Objects: ExtractObjectDetections(tags),
	}
}

// Comment extracts the JPEG COM marker comment string from the capture's
// JPEG data.  Returns an empty string if no COM marker is present.
func (cr *CaptureResult) Comment() string {
	return ExtractJpegComment(cr.JpegData)
}

// CommentTags parses the JPEG COM marker from the capture's JPEG data into
// a key=value map.
func (cr *CaptureResult) CommentTags() map[string]string {
	return ParseJpegCommentTags(ExtractJpegComment(cr.JpegData))
}

// Metadata performs a full extraction of all JPEG COM marker metadata from
// the capture's JPEG data.
func (cr *CaptureResult) Metadata() JpegCommentMetadata {
	return ParseJpegMetadata(cr.JpegData)
}

func parseBrackets(raw string) []string {
	var entries []string
	pos := 0
	for pos < len(raw) {
		open := strings.IndexByte(raw[pos:], '[')
		if open < 0 {
			break
		}
		open += pos
		close := strings.IndexByte(raw[open:], ']')
		if close < 0 {
			break
		}
		close += open
		entries = append(entries, raw[open+1:close])
		pos = close + 1
	}
	return entries
}

func atoi(s string) int {
	n := 0
	neg := false
	i := 0
	if len(s) > 0 && s[0] == '-' {
		neg = true
		i = 1
	}
	for ; i < len(s); i++ {
		if s[i] < '0' || s[i] > '9' {
			break
		}
		n = n*10 + int(s[i]-'0')
	}
	if neg {
		return -n
	}
	return n
}

"""
ITSCAM SDK - JPEG metadata extraction utilities

ITSCAM cameras embed recognition and classification metadata in the JPEG
COM marker (0xFF 0xFE) using a semicolon-delimited key=value format::

    Placa=ABC1234;CoordPlaca=100x200,150x40;ClassifierList=[1,95,50,100,300,200]

This module provides pure-Python helpers to extract and parse that metadata
without requiring the native library.

Typical keys for plate recognition:

    Placa             Plate text (multiple plates separated by ``_``)
    CoordPlaca        Bounding box per plate: ``XxY,WxH`` (separated by ``_``)
    CorPlaca          Plate color per plate (separated by ``_``)
    OCRCountryCode    Country code per plate (separated by ``_``)

Typical keys for vehicle classification:

    ClassifierList    ``[type,prob,x,y,w,h],[...]``  (prob 0-100)
    BMCList           ``["brand",bProb,"model",mProb,"color",cProb],[...]``
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Union


# ---------------------------------------------------------------------------
#  Low-level: extract the raw JPEG COM marker string
# ---------------------------------------------------------------------------

def extract_jpeg_comment(jpeg: Union[bytes, bytearray, memoryview]) -> str:
    """Extract the comment string from a JPEG's COM marker (0xFF 0xFE).

    Scans backwards from the end of the JPEG data for the COM marker, then
    reads the comment body.

    Args:
        jpeg: Raw JPEG bytes.

    Returns:
        The comment body as a string, or ``""`` if no COM marker was found.
    """
    data = bytes(jpeg) if isinstance(jpeg, memoryview) else jpeg
    size = len(data)
    if size < 4:
        return ""

    # Verify valid JPEG ending (EOI marker: 0xFF 0xD9)
    if data[size - 1] != 0xD9:
        return ""

    # Scan backwards for COM marker (0xFF 0xFE)
    for i in range(size - 1, 0, -1):
        if data[i - 1] == 0xFF and data[i] == 0xFE:
            len_pos = i + 1
            if len_pos > size - 2:
                return ""
            comment_len = (data[len_pos] << 8) | data[len_pos + 1]
            if comment_len < 2:
                return ""
            body_pos = len_pos + 2
            if body_pos > size:
                return ""
            body_len = comment_len - 2
            if body_len > size - body_pos:
                return ""
            return data[body_pos:body_pos + body_len].decode("ascii", errors="replace")

    return ""


# ---------------------------------------------------------------------------
#  Mid-level: parse comment string into a key=value tag dict
# ---------------------------------------------------------------------------

def parse_jpeg_comment_tags(comment: str) -> Dict[str, str]:
    """Parse a JPEG comment string into a key=value dictionary.

    The expected format is ``Key1=Value1;Key2=Value2;...``

    Args:
        comment: Raw comment string (from :func:`extract_jpeg_comment`).

    Returns:
        Dictionary mapping tag names to their values.
    """
    if not comment:
        return {}
    tags: Dict[str, str] = {}
    for field_str in comment.split(";"):
        eq = field_str.find("=")
        if eq > 0:
            tags[field_str[:eq]] = field_str[eq + 1:]
    return tags


# ---------------------------------------------------------------------------
#  High-level: structured recognition / detection results
# ---------------------------------------------------------------------------

@dataclass
class PlateRecognition:
    """Plate recognition result parsed from JPEG comment metadata."""
    plate: str = ""
    x: int = 0
    y: int = 0
    width: int = 0
    height: int = 0
    color: str = ""
    country_code: int = 0


@dataclass
class ObjectDetection:
    """Vehicle/object detection result parsed from JPEG comment metadata."""
    type: int = 0
    probability: float = 0.0
    x: int = 0
    y: int = 0
    width: int = 0
    height: int = 0
    brand: str = ""
    brand_prob: float = 0.0
    model: str = ""
    model_prob: float = 0.0
    color: str = ""
    color_prob: float = 0.0


def extract_plate_recognitions(tags: Dict[str, str]) -> List[PlateRecognition]:
    """Extract plate recognition results from parsed JPEG comment tags.

    Args:
        tags: Tag dictionary from :func:`parse_jpeg_comment_tags`.

    Returns:
        List of :class:`PlateRecognition` results (may be empty).
    """
    placa = tags.get("Placa", "")
    if not placa:
        return []

    plate_texts = placa.split("_")

    coord_texts = tags.get("CoordPlaca", "").split("_") if "CoordPlaca" in tags else []
    color_texts = tags.get("CorPlaca", "").split("_") if "CorPlaca" in tags else []
    country_texts = tags.get("OCRCountryCode", "").split("_") if "OCRCountryCode" in tags else []

    plates: List[PlateRecognition] = []
    for i, text in enumerate(plate_texts):
        if not text:
            continue
        pr = PlateRecognition(plate=text)

        if i < len(coord_texts) and coord_texts[i]:
            parts = coord_texts[i].split(",")
            if len(parts) >= 2:
                xy = parts[0].split("x")
                wh = parts[1].split("x")
                if len(xy) == 2:
                    pr.x = int(xy[0]) if xy[0].lstrip("-").isdigit() else 0
                    pr.y = int(xy[1]) if xy[1].lstrip("-").isdigit() else 0
                if len(wh) == 2:
                    pr.width = int(wh[0]) if wh[0].lstrip("-").isdigit() else 0
                    pr.height = int(wh[1]) if wh[1].lstrip("-").isdigit() else 0

        if i < len(color_texts):
            pr.color = color_texts[i]
        if i < len(country_texts) and country_texts[i].lstrip("-").isdigit():
            pr.country_code = int(country_texts[i])

        plates.append(pr)
    return plates


def extract_object_detections(tags: Dict[str, str]) -> List[ObjectDetection]:
    """Extract vehicle/object detection results from parsed JPEG comment tags.

    Args:
        tags: Tag dictionary from :func:`parse_jpeg_comment_tags`.

    Returns:
        List of :class:`ObjectDetection` results (may be empty).
    """
    cls_raw = tags.get("ClassifierList", "")
    if not cls_raw:
        return []

    def _parse_brackets(raw: str) -> List[str]:
        entries: List[str] = []
        pos = 0
        while pos < len(raw):
            o = raw.find("[", pos)
            if o < 0:
                break
            c = raw.find("]", o)
            if c < 0:
                break
            entries.append(raw[o + 1:c])
            pos = c + 1
        return entries

    cls_entries = _parse_brackets(cls_raw)
    bmc_entries = _parse_brackets(tags.get("BMCList", ""))

    detections: List[ObjectDetection] = []
    for i, entry in enumerate(cls_entries):
        parts = entry.split(",")
        if len(parts) < 6:
            continue
        try:
            od = ObjectDetection(
                type=int(parts[0]),
                probability=int(parts[1]) / 100.0,
                x=int(parts[2]),
                y=int(parts[3]),
                width=int(parts[4]),
                height=int(parts[5]),
            )
        except (ValueError, IndexError):
            continue

        if i < len(bmc_entries):
            cleaned = bmc_entries[i].replace('"', "")
            bmc_parts = cleaned.split(",")
            if len(bmc_parts) >= 6:
                try:
                    od.brand = bmc_parts[0]
                    od.brand_prob = int(bmc_parts[1]) / 100.0
                    od.model = bmc_parts[2]
                    od.model_prob = int(bmc_parts[3]) / 100.0
                    od.color = bmc_parts[4]
                    od.color_prob = int(bmc_parts[5]) / 100.0
                except (ValueError, IndexError):
                    pass

        detections.append(od)
    return detections


@dataclass
class JpegCommentMetadata:
    """All metadata extracted from a JPEG COM marker.

    Convenience container returned by :func:`parse_jpeg_metadata`.
    """
    comment: str = ""
    tags: Dict[str, str] = field(default_factory=dict)
    plates: List[PlateRecognition] = field(default_factory=list)
    objects: List[ObjectDetection] = field(default_factory=list)


def parse_jpeg_metadata(jpeg: Union[bytes, bytearray, memoryview]) -> JpegCommentMetadata:
    """One-shot extraction of all metadata from JPEG COM marker.

    Extracts the raw comment, parses it into key=value tags, and then
    extracts structured plate-recognition and object-detection results.

    Args:
        jpeg: Raw JPEG bytes.

    Returns:
        :class:`JpegCommentMetadata` with all parsed fields.
    """
    comment = extract_jpeg_comment(jpeg)
    if not comment:
        return JpegCommentMetadata()
    tags = parse_jpeg_comment_tags(comment)
    return JpegCommentMetadata(
        comment=comment,
        tags=tags,
        plates=extract_plate_recognitions(tags),
        objects=extract_object_detections(tags),
    )

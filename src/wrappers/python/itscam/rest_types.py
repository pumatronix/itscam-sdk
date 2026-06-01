# SPDX-License-Identifier: Proprietary
# Copyright (c) 2026 Pumatronix
#
# AUTO-GENERATED FILE -- DO NOT EDIT.
# Regenerate with `make codegen` (see tools/codegen/README.md).
#
# Generated from an OpenAPI 3.0 snapshot of the ITSCAM camera webapp.
# Edit tools/codegen/codegen.mjs and rerun, do not patch this output.
from dataclasses import dataclass
from typing import Optional, Any, List, TypeVar, Type, cast, Callable
from enum import Enum


T = TypeVar("T")
EnumT = TypeVar("EnumT", bound=Enum)


def from_int(x: Any) -> int:
    assert isinstance(x, int) and not isinstance(x, bool)
    return x


def from_none(x: Any) -> Any:
    assert x is None
    return x


def from_union(fs, x):
    for f in fs:
        try:
            return f(x)
        except:
            pass
    assert False


def from_float(x: Any) -> float:
    assert isinstance(x, (float, int)) and not isinstance(x, bool)
    return float(x)


def to_float(x: Any) -> float:
    assert isinstance(x, (int, float))
    return x


def to_class(c: Type[T], x: Any) -> dict:
    assert isinstance(x, c)
    return cast(Any, x).to_dict()


def from_bool(x: Any) -> bool:
    assert isinstance(x, bool)
    return x


def to_enum(c: Type[EnumT], x: Any) -> EnumT:
    assert isinstance(x, c)
    return x.value


def from_list(f: Callable[[Any], T], x: Any) -> List[T]:
    assert isinstance(x, list)
    return [f(y) for y in x]


def from_str(x: Any) -> str:
    assert isinstance(x, str)
    return x


@dataclass
class Exposition:
    preferred_shutter: Optional[int] = None
    update_factor: Optional[float] = None
    update_rate: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Exposition':
        assert isinstance(obj, dict)
        preferred_shutter = from_union([from_int, from_none], obj.get("preferredShutter"))
        update_factor = from_union([from_float, from_none], obj.get("updateFactor"))
        update_rate = from_union([from_int, from_none], obj.get("updateRate"))
        return Exposition(preferred_shutter, update_factor, update_rate)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.preferred_shutter is not None:
            result["preferredShutter"] = from_union([from_int, from_none], self.preferred_shutter)
        if self.update_factor is not None:
            result["updateFactor"] = from_union([to_float, from_none], self.update_factor)
        if self.update_rate is not None:
            result["updateRate"] = from_union([from_int, from_none], self.update_rate)
        return result


@dataclass
class AdvancedIris:
    update_rate: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'AdvancedIris':
        assert isinstance(obj, dict)
        update_rate = from_union([from_int, from_none], obj.get("updateRate"))
        return AdvancedIris(update_rate)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.update_rate is not None:
            result["updateRate"] = from_union([from_int, from_none], self.update_rate)
        return result


@dataclass
class AdvancedWhitebalance:
    update_rate: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'AdvancedWhitebalance':
        assert isinstance(obj, dict)
        update_rate = from_union([from_int, from_none], obj.get("updateRate"))
        return AdvancedWhitebalance(update_rate)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.update_rate is not None:
            result["updateRate"] = from_union([from_int, from_none], self.update_rate)
        return result


@dataclass
class Advanced:
    exposition: Optional[Exposition] = None
    iris: Optional[AdvancedIris] = None
    whitebalance: Optional[AdvancedWhitebalance] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Advanced':
        assert isinstance(obj, dict)
        exposition = from_union([Exposition.from_dict, from_none], obj.get("exposition"))
        iris = from_union([AdvancedIris.from_dict, from_none], obj.get("iris"))
        whitebalance = from_union([AdvancedWhitebalance.from_dict, from_none], obj.get("whitebalance"))
        return Advanced(exposition, iris, whitebalance)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.exposition is not None:
            result["exposition"] = from_union([lambda x: to_class(Exposition, x), from_none], self.exposition)
        if self.iris is not None:
            result["iris"] = from_union([lambda x: to_class(AdvancedIris, x), from_none], self.iris)
        if self.whitebalance is not None:
            result["whitebalance"] = from_union([lambda x: to_class(AdvancedWhitebalance, x), from_none], self.whitebalance)
        return result


@dataclass
class WhitebalanceClass:
    """Single RGB value in float format"""

    blue: Optional[float] = None
    green: Optional[float] = None
    red: Optional[float] = None

    @staticmethod
    def from_dict(obj: Any) -> 'WhitebalanceClass':
        assert isinstance(obj, dict)
        blue = from_union([from_float, from_none], obj.get("blue"))
        green = from_union([from_float, from_none], obj.get("green"))
        red = from_union([from_float, from_none], obj.get("red"))
        return WhitebalanceClass(blue, green, red)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.blue is not None:
            result["blue"] = from_union([to_float, from_none], self.blue)
        if self.green is not None:
            result["green"] = from_union([to_float, from_none], self.green)
        if self.red is not None:
            result["red"] = from_union([to_float, from_none], self.red)
        return result


@dataclass
class Color:
    """Camera color configuration fields"""

    blacklevel: Optional[int] = None
    brightness: Optional[int] = None
    contrast: Optional[int] = None
    gain: Optional[WhitebalanceClass] = None
    gamma: Optional[int] = None
    saturation: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Color':
        assert isinstance(obj, dict)
        blacklevel = from_union([from_int, from_none], obj.get("blacklevel"))
        brightness = from_union([from_int, from_none], obj.get("brightness"))
        contrast = from_union([from_int, from_none], obj.get("contrast"))
        gain = from_union([WhitebalanceClass.from_dict, from_none], obj.get("gain"))
        gamma = from_union([from_int, from_none], obj.get("gamma"))
        saturation = from_union([from_int, from_none], obj.get("saturation"))
        return Color(blacklevel, brightness, contrast, gain, gamma, saturation)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.blacklevel is not None:
            result["blacklevel"] = from_union([from_int, from_none], self.blacklevel)
        if self.brightness is not None:
            result["brightness"] = from_union([from_int, from_none], self.brightness)
        if self.contrast is not None:
            result["contrast"] = from_union([from_int, from_none], self.contrast)
        if self.gain is not None:
            result["gain"] = from_union([lambda x: to_class(WhitebalanceClass, x), from_none], self.gain)
        if self.gamma is not None:
            result["gamma"] = from_union([from_int, from_none], self.gamma)
        if self.saturation is not None:
            result["saturation"] = from_union([from_int, from_none], self.saturation)
        return result


@dataclass
class ShutterClass:
    """Shutter attributes"""

    automatic: Optional[bool] = None
    fixed_value: Optional[int] = None
    max_value: Optional[int] = None
    min_value: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ShutterClass':
        assert isinstance(obj, dict)
        automatic = from_union([from_bool, from_none], obj.get("automatic"))
        fixed_value = from_union([from_int, from_none], obj.get("fixedValue"))
        max_value = from_union([from_int, from_none], obj.get("maxValue"))
        min_value = from_union([from_int, from_none], obj.get("minValue"))
        return ShutterClass(automatic, fixed_value, max_value, min_value)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.automatic is not None:
            result["automatic"] = from_union([from_bool, from_none], self.automatic)
        if self.fixed_value is not None:
            result["fixedValue"] = from_union([from_int, from_none], self.fixed_value)
        if self.max_value is not None:
            result["maxValue"] = from_union([from_int, from_none], self.max_value)
        if self.min_value is not None:
            result["minValue"] = from_union([from_int, from_none], self.min_value)
        return result


@dataclass
class ExposureIris:
    automatic: Optional[bool] = None
    fixed_value: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ExposureIris':
        assert isinstance(obj, dict)
        automatic = from_union([from_bool, from_none], obj.get("automatic"))
        fixed_value = from_union([from_int, from_none], obj.get("fixedValue"))
        return ExposureIris(automatic, fixed_value)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.automatic is not None:
            result["automatic"] = from_union([from_bool, from_none], self.automatic)
        if self.fixed_value is not None:
            result["fixedValue"] = from_union([from_int, from_none], self.fixed_value)
        return result


class Mode(Enum):
    DISABLED = "disabled"
    FAST = "fast"
    NORMAL = "normal"
    SLOW = "slow"


@dataclass
class Roi1Class:
    enabled: Optional[bool] = None
    x0: Optional[int] = None
    x1: Optional[int] = None
    x2: Optional[int] = None
    x3: Optional[int] = None
    y0: Optional[int] = None
    y1: Optional[int] = None
    y2: Optional[int] = None
    y3: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Roi1Class':
        assert isinstance(obj, dict)
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        x0 = from_union([from_int, from_none], obj.get("x0"))
        x1 = from_union([from_int, from_none], obj.get("x1"))
        x2 = from_union([from_int, from_none], obj.get("x2"))
        x3 = from_union([from_int, from_none], obj.get("x3"))
        y0 = from_union([from_int, from_none], obj.get("y0"))
        y1 = from_union([from_int, from_none], obj.get("y1"))
        y2 = from_union([from_int, from_none], obj.get("y2"))
        y3 = from_union([from_int, from_none], obj.get("y3"))
        return Roi1Class(enabled, x0, x1, x2, x3, y0, y1, y2, y3)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.x0 is not None:
            result["x0"] = from_union([from_int, from_none], self.x0)
        if self.x1 is not None:
            result["x1"] = from_union([from_int, from_none], self.x1)
        if self.x2 is not None:
            result["x2"] = from_union([from_int, from_none], self.x2)
        if self.x3 is not None:
            result["x3"] = from_union([from_int, from_none], self.x3)
        if self.y0 is not None:
            result["y0"] = from_union([from_int, from_none], self.y0)
        if self.y1 is not None:
            result["y1"] = from_union([from_int, from_none], self.y1)
        if self.y2 is not None:
            result["y2"] = from_union([from_int, from_none], self.y2)
        if self.y3 is not None:
            result["y3"] = from_union([from_int, from_none], self.y3)
        return result


@dataclass
class Level:
    hold_time: Optional[int] = None
    mode: Optional[Mode] = None
    roi: Optional[Roi1Class] = None
    target_value: Optional[float] = None
    update_rate: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Level':
        assert isinstance(obj, dict)
        hold_time = from_union([from_int, from_none], obj.get("holdTime"))
        mode = from_union([Mode, from_none], obj.get("mode"))
        roi = from_union([Roi1Class.from_dict, from_none], obj.get("roi"))
        target_value = from_union([from_float, from_none], obj.get("targetValue"))
        update_rate = from_union([from_int, from_none], obj.get("updateRate"))
        return Level(hold_time, mode, roi, target_value, update_rate)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.hold_time is not None:
            result["holdTime"] = from_union([from_int, from_none], self.hold_time)
        if self.mode is not None:
            result["mode"] = from_union([lambda x: to_enum(Mode, x), from_none], self.mode)
        if self.roi is not None:
            result["roi"] = from_union([lambda x: to_class(Roi1Class, x), from_none], self.roi)
        if self.target_value is not None:
            result["targetValue"] = from_union([to_float, from_none], self.target_value)
        if self.update_rate is not None:
            result["updateRate"] = from_union([from_int, from_none], self.update_rate)
        return result


@dataclass
class Exposure:
    """Camera exposure configuration fields"""

    gain: Optional[ShutterClass] = None
    iris: Optional[ExposureIris] = None
    level: Optional[Level] = None
    shutter: Optional[ShutterClass] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Exposure':
        assert isinstance(obj, dict)
        gain = from_union([ShutterClass.from_dict, from_none], obj.get("gain"))
        iris = from_union([ExposureIris.from_dict, from_none], obj.get("iris"))
        level = from_union([Level.from_dict, from_none], obj.get("level"))
        shutter = from_union([ShutterClass.from_dict, from_none], obj.get("shutter"))
        return Exposure(gain, iris, level, shutter)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.gain is not None:
            result["gain"] = from_union([lambda x: to_class(ShutterClass, x), from_none], self.gain)
        if self.iris is not None:
            result["iris"] = from_union([lambda x: to_class(ExposureIris, x), from_none], self.iris)
        if self.level is not None:
            result["level"] = from_union([lambda x: to_class(Level, x), from_none], self.level)
        if self.shutter is not None:
            result["shutter"] = from_union([lambda x: to_class(ShutterClass, x), from_none], self.shutter)
        return result


@dataclass
class Hdr:
    enable: Optional[bool] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Hdr':
        assert isinstance(obj, dict)
        enable = from_union([from_bool, from_none], obj.get("enable"))
        return Hdr(enable)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.enable is not None:
            result["enable"] = from_union([from_bool, from_none], self.enable)
        return result


@dataclass
class ProfileConfigLens:
    """Camera lens configuration fields"""

    exchanger: Optional[bool] = None
    focus: Optional[int] = None
    zf_mirror_profile0: Optional[bool] = None
    zoom: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ProfileConfigLens':
        assert isinstance(obj, dict)
        exchanger = from_union([from_bool, from_none], obj.get("exchanger"))
        focus = from_union([from_int, from_none], obj.get("focus"))
        zf_mirror_profile0 = from_union([from_bool, from_none], obj.get("zfMirrorProfile0"))
        zoom = from_union([from_int, from_none], obj.get("zoom"))
        return ProfileConfigLens(exchanger, focus, zf_mirror_profile0, zoom)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.exchanger is not None:
            result["exchanger"] = from_union([from_bool, from_none], self.exchanger)
        if self.focus is not None:
            result["focus"] = from_union([from_int, from_none], self.focus)
        if self.zf_mirror_profile0 is not None:
            result["zfMirrorProfile0"] = from_union([from_bool, from_none], self.zf_mirror_profile0)
        if self.zoom is not None:
            result["zoom"] = from_union([from_int, from_none], self.zoom)
        return result


@dataclass
class MOVFilter:
    enabled: Optional[bool] = None
    only_check: Optional[bool] = None
    roi: Optional[Roi1Class] = None
    threshold: Optional[float] = None

    @staticmethod
    def from_dict(obj: Any) -> 'MOVFilter':
        assert isinstance(obj, dict)
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        only_check = from_union([from_bool, from_none], obj.get("onlyCheck"))
        roi = from_union([Roi1Class.from_dict, from_none], obj.get("roi"))
        threshold = from_union([from_float, from_none], obj.get("threshold"))
        return MOVFilter(enabled, only_check, roi, threshold)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.only_check is not None:
            result["onlyCheck"] = from_union([from_bool, from_none], self.only_check)
        if self.roi is not None:
            result["roi"] = from_union([lambda x: to_class(Roi1Class, x), from_none], self.roi)
        if self.threshold is not None:
            result["threshold"] = from_union([to_float, from_none], self.threshold)
        return result


@dataclass
class Power:
    out: Optional[int] = None
    percent: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Power':
        assert isinstance(obj, dict)
        out = from_union([from_int, from_none], obj.get("out"))
        percent = from_union([from_int, from_none], obj.get("percent"))
        return Power(out, percent)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.out is not None:
            result["out"] = from_union([from_int, from_none], self.out)
        if self.percent is not None:
            result["percent"] = from_union([from_int, from_none], self.percent)
        return result


@dataclass
class Flash:
    """Camera flash configuration"""

    power: Optional[List[Power]] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Flash':
        assert isinstance(obj, dict)
        power = from_union([lambda x: from_list(Power.from_dict, x), from_none], obj.get("power"))
        return Flash(power)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.power is not None:
            result["power"] = from_union([lambda x: from_list(lambda x: to_class(Power, x), x), from_none], self.power)
        return result


@dataclass
class SettingGain:
    percentage_of_current: Optional[bool] = None
    value: Optional[float] = None

    @staticmethod
    def from_dict(obj: Any) -> 'SettingGain':
        assert isinstance(obj, dict)
        percentage_of_current = from_union([from_bool, from_none], obj.get("percentageOfCurrent"))
        value = from_union([from_float, from_none], obj.get("value"))
        return SettingGain(percentage_of_current, value)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.percentage_of_current is not None:
            result["percentageOfCurrent"] = from_union([from_bool, from_none], self.percentage_of_current)
        if self.value is not None:
            result["value"] = from_union([to_float, from_none], self.value)
        return result


@dataclass
class Shutter:
    percentage_of_current: Optional[bool] = None
    value: Optional[float] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Shutter':
        assert isinstance(obj, dict)
        percentage_of_current = from_union([from_bool, from_none], obj.get("percentageOfCurrent"))
        value = from_union([from_float, from_none], obj.get("value"))
        return Shutter(percentage_of_current, value)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.percentage_of_current is not None:
            result["percentageOfCurrent"] = from_union([from_bool, from_none], self.percentage_of_current)
        if self.value is not None:
            result["value"] = from_union([to_float, from_none], self.value)
        return result


@dataclass
class MultipleExposuresConfig:
    """Multiple exposures configuration"""

    flash: Optional[Flash] = None
    gain: Optional[SettingGain] = None
    shutter: Optional[Shutter] = None

    @staticmethod
    def from_dict(obj: Any) -> 'MultipleExposuresConfig':
        assert isinstance(obj, dict)
        flash = from_union([Flash.from_dict, from_none], obj.get("flash"))
        gain = from_union([SettingGain.from_dict, from_none], obj.get("gain"))
        shutter = from_union([Shutter.from_dict, from_none], obj.get("shutter"))
        return MultipleExposuresConfig(flash, gain, shutter)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.flash is not None:
            result["flash"] = from_union([lambda x: to_class(Flash, x), from_none], self.flash)
        if self.gain is not None:
            result["gain"] = from_union([lambda x: to_class(SettingGain, x), from_none], self.gain)
        if self.shutter is not None:
            result["shutter"] = from_union([lambda x: to_class(Shutter, x), from_none], self.shutter)
        return result


@dataclass
class MultipleExposures:
    enabled: Optional[bool] = None
    settings: Optional[List[MultipleExposuresConfig]] = None

    @staticmethod
    def from_dict(obj: Any) -> 'MultipleExposures':
        assert isinstance(obj, dict)
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        settings = from_union([lambda x: from_list(MultipleExposuresConfig.from_dict, x), from_none], obj.get("settings"))
        return MultipleExposures(enabled, settings)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.settings is not None:
            result["settings"] = from_union([lambda x: from_list(lambda x: to_class(MultipleExposuresConfig, x), x), from_none], self.settings)
        return result


@dataclass
class Overlay:
    enable: Optional[bool] = None
    text: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Overlay':
        assert isinstance(obj, dict)
        enable = from_union([from_bool, from_none], obj.get("enable"))
        text = from_union([from_str, from_none], obj.get("text"))
        return Overlay(enable, text)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.enable is not None:
            result["enable"] = from_union([from_bool, from_none], self.enable)
        if self.text is not None:
            result["text"] = from_union([from_str, from_none], self.text)
        return result


@dataclass
class Lower:
    end_time: Optional[str] = None
    hold_time: Optional[int] = None
    level: Optional[float] = None
    profile: Optional[int] = None
    start_time: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Lower':
        assert isinstance(obj, dict)
        end_time = from_union([from_str, from_none], obj.get("endTime"))
        hold_time = from_union([from_int, from_none], obj.get("holdTime"))
        level = from_union([from_float, from_none], obj.get("level"))
        profile = from_union([from_int, from_none], obj.get("profile"))
        start_time = from_union([from_str, from_none], obj.get("startTime"))
        return Lower(end_time, hold_time, level, profile, start_time)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.end_time is not None:
            result["endTime"] = from_union([from_str, from_none], self.end_time)
        if self.hold_time is not None:
            result["holdTime"] = from_union([from_int, from_none], self.hold_time)
        if self.level is not None:
            result["level"] = from_union([to_float, from_none], self.level)
        if self.profile is not None:
            result["profile"] = from_union([from_int, from_none], self.profile)
        if self.start_time is not None:
            result["startTime"] = from_union([from_str, from_none], self.start_time)
        return result


@dataclass
class Transitions:
    """Camera profile transition configuration"""

    lower: Optional[Lower] = None
    upper: Optional[Lower] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Transitions':
        assert isinstance(obj, dict)
        lower = from_union([Lower.from_dict, from_none], obj.get("lower"))
        upper = from_union([Lower.from_dict, from_none], obj.get("upper"))
        return Transitions(lower, upper)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.lower is not None:
            result["lower"] = from_union([lambda x: to_class(Lower, x), from_none], self.lower)
        if self.upper is not None:
            result["upper"] = from_union([lambda x: to_class(Lower, x), from_none], self.upper)
        return result


@dataclass
class Trigger:
    enabled: Optional[bool] = None
    event: Optional[str] = None
    minimum_interval: Optional[int] = None
    port: Optional[int] = None
    roi: Optional[Roi1Class] = None
    threshold: Optional[float] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Trigger':
        assert isinstance(obj, dict)
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        event = from_union([from_str, from_none], obj.get("event"))
        minimum_interval = from_union([from_int, from_none], obj.get("minimumInterval"))
        port = from_union([from_int, from_none], obj.get("port"))
        roi = from_union([Roi1Class.from_dict, from_none], obj.get("roi"))
        threshold = from_union([from_float, from_none], obj.get("threshold"))
        return Trigger(enabled, event, minimum_interval, port, roi, threshold)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.event is not None:
            result["event"] = from_union([from_str, from_none], self.event)
        if self.minimum_interval is not None:
            result["minimumInterval"] = from_union([from_int, from_none], self.minimum_interval)
        if self.port is not None:
            result["port"] = from_union([from_int, from_none], self.port)
        if self.roi is not None:
            result["roi"] = from_union([lambda x: to_class(Roi1Class, x), from_none], self.roi)
        if self.threshold is not None:
            result["threshold"] = from_union([to_float, from_none], self.threshold)
        return result


@dataclass
class ProfileConfigWhitebalance:
    """Camera white balance configuration fields"""

    automatic: Optional[bool] = None
    weights: Optional[WhitebalanceClass] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ProfileConfigWhitebalance':
        assert isinstance(obj, dict)
        automatic = from_union([from_bool, from_none], obj.get("automatic"))
        weights = from_union([WhitebalanceClass.from_dict, from_none], obj.get("weights"))
        return ProfileConfigWhitebalance(automatic, weights)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.automatic is not None:
            result["automatic"] = from_union([from_bool, from_none], self.automatic)
        if self.weights is not None:
            result["weights"] = from_union([lambda x: to_class(WhitebalanceClass, x), from_none], self.weights)
        return result


@dataclass
class ProfileConfig:
    """Camera profile configuration"""

    id: int
    active: Optional[bool] = None
    advanced: Optional[Advanced] = None
    color: Optional[Color] = None
    description: Optional[str] = None
    exposure: Optional[Exposure] = None
    hdr: Optional[Hdr] = None
    lens: Optional[ProfileConfigLens] = None
    mov_filter: Optional[MOVFilter] = None
    multiple_exposures: Optional[MultipleExposures] = None
    name: Optional[str] = None
    overlay: Optional[Overlay] = None
    transitions: Optional[Transitions] = None
    trigger: Optional[Trigger] = None
    whitebalance: Optional[ProfileConfigWhitebalance] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ProfileConfig':
        assert isinstance(obj, dict)
        id = from_int(obj.get("id"))
        active = from_union([from_bool, from_none], obj.get("active"))
        advanced = from_union([Advanced.from_dict, from_none], obj.get("advanced"))
        color = from_union([Color.from_dict, from_none], obj.get("color"))
        description = from_union([from_str, from_none], obj.get("description"))
        exposure = from_union([Exposure.from_dict, from_none], obj.get("exposure"))
        hdr = from_union([Hdr.from_dict, from_none], obj.get("hdr"))
        lens = from_union([ProfileConfigLens.from_dict, from_none], obj.get("lens"))
        mov_filter = from_union([MOVFilter.from_dict, from_none], obj.get("movFilter"))
        multiple_exposures = from_union([MultipleExposures.from_dict, from_none], obj.get("multipleExposures"))
        name = from_union([from_str, from_none], obj.get("name"))
        overlay = from_union([Overlay.from_dict, from_none], obj.get("overlay"))
        transitions = from_union([Transitions.from_dict, from_none], obj.get("transitions"))
        trigger = from_union([Trigger.from_dict, from_none], obj.get("trigger"))
        whitebalance = from_union([ProfileConfigWhitebalance.from_dict, from_none], obj.get("whitebalance"))
        return ProfileConfig(id, active, advanced, color, description, exposure, hdr, lens, mov_filter, multiple_exposures, name, overlay, transitions, trigger, whitebalance)

    def to_dict(self) -> dict:
        result: dict = {}
        result["id"] = from_int(self.id)
        if self.active is not None:
            result["active"] = from_union([from_bool, from_none], self.active)
        if self.advanced is not None:
            result["advanced"] = from_union([lambda x: to_class(Advanced, x), from_none], self.advanced)
        if self.color is not None:
            result["color"] = from_union([lambda x: to_class(Color, x), from_none], self.color)
        if self.description is not None:
            result["description"] = from_union([from_str, from_none], self.description)
        if self.exposure is not None:
            result["exposure"] = from_union([lambda x: to_class(Exposure, x), from_none], self.exposure)
        if self.hdr is not None:
            result["hdr"] = from_union([lambda x: to_class(Hdr, x), from_none], self.hdr)
        if self.lens is not None:
            result["lens"] = from_union([lambda x: to_class(ProfileConfigLens, x), from_none], self.lens)
        if self.mov_filter is not None:
            result["movFilter"] = from_union([lambda x: to_class(MOVFilter, x), from_none], self.mov_filter)
        if self.multiple_exposures is not None:
            result["multipleExposures"] = from_union([lambda x: to_class(MultipleExposures, x), from_none], self.multiple_exposures)
        if self.name is not None:
            result["name"] = from_union([from_str, from_none], self.name)
        if self.overlay is not None:
            result["overlay"] = from_union([lambda x: to_class(Overlay, x), from_none], self.overlay)
        if self.transitions is not None:
            result["transitions"] = from_union([lambda x: to_class(Transitions, x), from_none], self.transitions)
        if self.trigger is not None:
            result["trigger"] = from_union([lambda x: to_class(Trigger, x), from_none], self.trigger)
        if self.whitebalance is not None:
            result["whitebalance"] = from_union([lambda x: to_class(ProfileConfigWhitebalance, x), from_none], self.whitebalance)
        return result


@dataclass
class OcrConfigOcr:
    avg_char_height: Optional[int] = None
    avg_plate_angle: Optional[float] = None
    avg_plate_slant: Optional[float] = None
    classifier_expansion: Optional[float] = None
    country_code: Optional[int] = None
    enabled: Optional[bool] = None
    licensed: Optional[bool] = None
    max_char_height: Optional[int] = None
    max_low_prob_chars: Optional[int] = None
    max_plates: Optional[int] = None
    min_char_height: Optional[int] = None
    min_prob_per_char: Optional[float] = None
    processing_mode: Optional[int] = None
    processing_queue: Optional[int] = None
    processing_threads: Optional[int] = None
    processing_timeout: Optional[int] = None
    roi: Optional[Roi1Class] = None
    use_classifier_result: Optional[bool] = None
    vehicle_type: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'OcrConfigOcr':
        assert isinstance(obj, dict)
        avg_char_height = from_union([from_int, from_none], obj.get("avgCharHeight"))
        avg_plate_angle = from_union([from_float, from_none], obj.get("avgPlateAngle"))
        avg_plate_slant = from_union([from_float, from_none], obj.get("avgPlateSlant"))
        classifier_expansion = from_union([from_float, from_none], obj.get("classifierExpansion"))
        country_code = from_union([from_int, from_none], obj.get("countryCode"))
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        licensed = from_union([from_bool, from_none], obj.get("licensed"))
        max_char_height = from_union([from_int, from_none], obj.get("maxCharHeight"))
        max_low_prob_chars = from_union([from_int, from_none], obj.get("maxLowProbChars"))
        max_plates = from_union([from_int, from_none], obj.get("maxPlates"))
        min_char_height = from_union([from_int, from_none], obj.get("minCharHeight"))
        min_prob_per_char = from_union([from_float, from_none], obj.get("minProbPerChar"))
        processing_mode = from_union([from_int, from_none], obj.get("processingMode"))
        processing_queue = from_union([from_int, from_none], obj.get("processingQueue"))
        processing_threads = from_union([from_int, from_none], obj.get("processingThreads"))
        processing_timeout = from_union([from_int, from_none], obj.get("processingTimeout"))
        roi = from_union([Roi1Class.from_dict, from_none], obj.get("roi"))
        use_classifier_result = from_union([from_bool, from_none], obj.get("useClassifierResult"))
        vehicle_type = from_union([from_int, from_none], obj.get("vehicleType"))
        return OcrConfigOcr(avg_char_height, avg_plate_angle, avg_plate_slant, classifier_expansion, country_code, enabled, licensed, max_char_height, max_low_prob_chars, max_plates, min_char_height, min_prob_per_char, processing_mode, processing_queue, processing_threads, processing_timeout, roi, use_classifier_result, vehicle_type)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.avg_char_height is not None:
            result["avgCharHeight"] = from_union([from_int, from_none], self.avg_char_height)
        if self.avg_plate_angle is not None:
            result["avgPlateAngle"] = from_union([to_float, from_none], self.avg_plate_angle)
        if self.avg_plate_slant is not None:
            result["avgPlateSlant"] = from_union([to_float, from_none], self.avg_plate_slant)
        if self.classifier_expansion is not None:
            result["classifierExpansion"] = from_union([to_float, from_none], self.classifier_expansion)
        if self.country_code is not None:
            result["countryCode"] = from_union([from_int, from_none], self.country_code)
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.licensed is not None:
            result["licensed"] = from_union([from_bool, from_none], self.licensed)
        if self.max_char_height is not None:
            result["maxCharHeight"] = from_union([from_int, from_none], self.max_char_height)
        if self.max_low_prob_chars is not None:
            result["maxLowProbChars"] = from_union([from_int, from_none], self.max_low_prob_chars)
        if self.max_plates is not None:
            result["maxPlates"] = from_union([from_int, from_none], self.max_plates)
        if self.min_char_height is not None:
            result["minCharHeight"] = from_union([from_int, from_none], self.min_char_height)
        if self.min_prob_per_char is not None:
            result["minProbPerChar"] = from_union([to_float, from_none], self.min_prob_per_char)
        if self.processing_mode is not None:
            result["processingMode"] = from_union([from_int, from_none], self.processing_mode)
        if self.processing_queue is not None:
            result["processingQueue"] = from_union([from_int, from_none], self.processing_queue)
        if self.processing_threads is not None:
            result["processingThreads"] = from_union([from_int, from_none], self.processing_threads)
        if self.processing_timeout is not None:
            result["processingTimeout"] = from_union([from_int, from_none], self.processing_timeout)
        if self.roi is not None:
            result["roi"] = from_union([lambda x: to_class(Roi1Class, x), from_none], self.roi)
        if self.use_classifier_result is not None:
            result["useClassifierResult"] = from_union([from_bool, from_none], self.use_classifier_result)
        if self.vehicle_type is not None:
            result["vehicleType"] = from_union([from_int, from_none], self.vehicle_type)
        return result


@dataclass
class OcrConfig:
    """Ocr service configuration"""

    ocr: Optional[OcrConfigOcr] = None

    @staticmethod
    def from_dict(obj: Any) -> 'OcrConfig':
        assert isinstance(obj, dict)
        ocr = from_union([OcrConfigOcr.from_dict, from_none], obj.get("ocr"))
        return OcrConfig(ocr)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.ocr is not None:
            result["ocr"] = from_union([lambda x: to_class(OcrConfigOcr, x), from_none], self.ocr)
        return result


@dataclass
class Voting:
    enabled: Optional[bool] = None
    forward_without_plate_if_tracker: Optional[bool] = None
    keep_best_only: Optional[bool] = None
    max_diff_chars: Optional[int] = None
    roi1: Optional[Roi1Class] = None
    roi2: Optional[Roi1Class] = None
    same_plate_debounce: Optional[int] = None
    use_classifier: Optional[bool] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Voting':
        assert isinstance(obj, dict)
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        forward_without_plate_if_tracker = from_union([from_bool, from_none], obj.get("forwardWithoutPlateIfTracker"))
        keep_best_only = from_union([from_bool, from_none], obj.get("keepBestOnly"))
        max_diff_chars = from_union([from_int, from_none], obj.get("maxDiffChars"))
        roi1 = from_union([Roi1Class.from_dict, from_none], obj.get("roi1"))
        roi2 = from_union([Roi1Class.from_dict, from_none], obj.get("roi2"))
        same_plate_debounce = from_union([from_int, from_none], obj.get("samePlateDebounce"))
        use_classifier = from_union([from_bool, from_none], obj.get("useClassifier"))
        return Voting(enabled, forward_without_plate_if_tracker, keep_best_only, max_diff_chars, roi1, roi2, same_plate_debounce, use_classifier)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.forward_without_plate_if_tracker is not None:
            result["forwardWithoutPlateIfTracker"] = from_union([from_bool, from_none], self.forward_without_plate_if_tracker)
        if self.keep_best_only is not None:
            result["keepBestOnly"] = from_union([from_bool, from_none], self.keep_best_only)
        if self.max_diff_chars is not None:
            result["maxDiffChars"] = from_union([from_int, from_none], self.max_diff_chars)
        if self.roi1 is not None:
            result["roi1"] = from_union([lambda x: to_class(Roi1Class, x), from_none], self.roi1)
        if self.roi2 is not None:
            result["roi2"] = from_union([lambda x: to_class(Roi1Class, x), from_none], self.roi2)
        if self.same_plate_debounce is not None:
            result["samePlateDebounce"] = from_union([from_int, from_none], self.same_plate_debounce)
        if self.use_classifier is not None:
            result["useClassifier"] = from_union([from_bool, from_none], self.use_classifier)
        return result


@dataclass
class AnalyticsConfig:
    """Plate Analytics service configuration"""

    voting: Optional[Voting] = None

    @staticmethod
    def from_dict(obj: Any) -> 'AnalyticsConfig':
        assert isinstance(obj, dict)
        voting = from_union([Voting.from_dict, from_none], obj.get("voting"))
        return AnalyticsConfig(voting)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.voting is not None:
            result["voting"] = from_union([lambda x: to_class(Voting, x), from_none], self.voting)
        return result


@dataclass
class SpeedCalibrationRegion1:
    """Classifier based speed calibration region;"""

    p0_top1_sz: Optional[float] = None
    x0: Optional[int] = None
    x1: Optional[int] = None
    x2: Optional[int] = None
    y0: Optional[int] = None
    y1: Optional[int] = None
    y2: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'SpeedCalibrationRegion1':
        assert isinstance(obj, dict)
        p0_top1_sz = from_union([from_float, from_none], obj.get("p0top1sz"))
        x0 = from_union([from_int, from_none], obj.get("x0"))
        x1 = from_union([from_int, from_none], obj.get("x1"))
        x2 = from_union([from_int, from_none], obj.get("x2"))
        y0 = from_union([from_int, from_none], obj.get("y0"))
        y1 = from_union([from_int, from_none], obj.get("y1"))
        y2 = from_union([from_int, from_none], obj.get("y2"))
        return SpeedCalibrationRegion1(p0_top1_sz, x0, x1, x2, y0, y1, y2)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.p0_top1_sz is not None:
            result["p0top1sz"] = from_union([to_float, from_none], self.p0_top1_sz)
        if self.x0 is not None:
            result["x0"] = from_union([from_int, from_none], self.x0)
        if self.x1 is not None:
            result["x1"] = from_union([from_int, from_none], self.x1)
        if self.x2 is not None:
            result["x2"] = from_union([from_int, from_none], self.x2)
        if self.y0 is not None:
            result["y0"] = from_union([from_int, from_none], self.y0)
        if self.y1 is not None:
            result["y1"] = from_union([from_int, from_none], self.y1)
        if self.y2 is not None:
            result["y2"] = from_union([from_int, from_none], self.y2)
        return result


@dataclass
class TriggerRegion0:
    """Classifier based trigger region; * dir: top-bottom, bottom-top, disabled"""

    dir: Optional[str] = None
    x0: Optional[int] = None
    x1: Optional[int] = None
    y0: Optional[int] = None
    y1: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'TriggerRegion0':
        assert isinstance(obj, dict)
        dir = from_union([from_str, from_none], obj.get("dir"))
        x0 = from_union([from_int, from_none], obj.get("x0"))
        x1 = from_union([from_int, from_none], obj.get("x1"))
        y0 = from_union([from_int, from_none], obj.get("y0"))
        y1 = from_union([from_int, from_none], obj.get("y1"))
        return TriggerRegion0(dir, x0, x1, y0, y1)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.dir is not None:
            result["dir"] = from_union([from_str, from_none], self.dir)
        if self.x0 is not None:
            result["x0"] = from_union([from_int, from_none], self.x0)
        if self.x1 is not None:
            result["x1"] = from_union([from_int, from_none], self.x1)
        if self.y0 is not None:
            result["y0"] = from_union([from_int, from_none], self.y0)
        if self.y1 is not None:
            result["y1"] = from_union([from_int, from_none], self.y1)
        return result


@dataclass
class ClassifierConfigClassifier:
    enable_characteristics: Optional[bool] = None
    enabled: Optional[bool] = None
    enable_speed: Optional[bool] = None
    first_only: Optional[bool] = None
    licensed: Optional[bool] = None
    min_probability: Optional[float] = None
    model_type: Optional[int] = None
    processing_queue: Optional[int] = None
    processing_threads: Optional[int] = None
    scene_type: Optional[int] = None
    speed_calibration_region1: Optional[SpeedCalibrationRegion1] = None
    speed_calibration_region2: Optional[SpeedCalibrationRegion1] = None
    trigger_enabled: Optional[bool] = None
    trigger_region0: Optional[TriggerRegion0] = None
    trigger_region1: Optional[TriggerRegion0] = None
    trigger_region2: Optional[TriggerRegion0] = None
    trigger_region3: Optional[TriggerRegion0] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ClassifierConfigClassifier':
        assert isinstance(obj, dict)
        enable_characteristics = from_union([from_bool, from_none], obj.get("enableCharacteristics"))
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        enable_speed = from_union([from_bool, from_none], obj.get("enableSpeed"))
        first_only = from_union([from_bool, from_none], obj.get("firstOnly"))
        licensed = from_union([from_bool, from_none], obj.get("licensed"))
        min_probability = from_union([from_float, from_none], obj.get("minProbability"))
        model_type = from_union([from_int, from_none], obj.get("modelType"))
        processing_queue = from_union([from_int, from_none], obj.get("processingQueue"))
        processing_threads = from_union([from_int, from_none], obj.get("processingThreads"))
        scene_type = from_union([from_int, from_none], obj.get("sceneType"))
        speed_calibration_region1 = from_union([SpeedCalibrationRegion1.from_dict, from_none], obj.get("speedCalibrationRegion1"))
        speed_calibration_region2 = from_union([SpeedCalibrationRegion1.from_dict, from_none], obj.get("speedCalibrationRegion2"))
        trigger_enabled = from_union([from_bool, from_none], obj.get("triggerEnabled"))
        trigger_region0 = from_union([TriggerRegion0.from_dict, from_none], obj.get("triggerRegion0"))
        trigger_region1 = from_union([TriggerRegion0.from_dict, from_none], obj.get("triggerRegion1"))
        trigger_region2 = from_union([TriggerRegion0.from_dict, from_none], obj.get("triggerRegion2"))
        trigger_region3 = from_union([TriggerRegion0.from_dict, from_none], obj.get("triggerRegion3"))
        return ClassifierConfigClassifier(enable_characteristics, enabled, enable_speed, first_only, licensed, min_probability, model_type, processing_queue, processing_threads, scene_type, speed_calibration_region1, speed_calibration_region2, trigger_enabled, trigger_region0, trigger_region1, trigger_region2, trigger_region3)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.enable_characteristics is not None:
            result["enableCharacteristics"] = from_union([from_bool, from_none], self.enable_characteristics)
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.enable_speed is not None:
            result["enableSpeed"] = from_union([from_bool, from_none], self.enable_speed)
        if self.first_only is not None:
            result["firstOnly"] = from_union([from_bool, from_none], self.first_only)
        if self.licensed is not None:
            result["licensed"] = from_union([from_bool, from_none], self.licensed)
        if self.min_probability is not None:
            result["minProbability"] = from_union([to_float, from_none], self.min_probability)
        if self.model_type is not None:
            result["modelType"] = from_union([from_int, from_none], self.model_type)
        if self.processing_queue is not None:
            result["processingQueue"] = from_union([from_int, from_none], self.processing_queue)
        if self.processing_threads is not None:
            result["processingThreads"] = from_union([from_int, from_none], self.processing_threads)
        if self.scene_type is not None:
            result["sceneType"] = from_union([from_int, from_none], self.scene_type)
        if self.speed_calibration_region1 is not None:
            result["speedCalibrationRegion1"] = from_union([lambda x: to_class(SpeedCalibrationRegion1, x), from_none], self.speed_calibration_region1)
        if self.speed_calibration_region2 is not None:
            result["speedCalibrationRegion2"] = from_union([lambda x: to_class(SpeedCalibrationRegion1, x), from_none], self.speed_calibration_region2)
        if self.trigger_enabled is not None:
            result["triggerEnabled"] = from_union([from_bool, from_none], self.trigger_enabled)
        if self.trigger_region0 is not None:
            result["triggerRegion0"] = from_union([lambda x: to_class(TriggerRegion0, x), from_none], self.trigger_region0)
        if self.trigger_region1 is not None:
            result["triggerRegion1"] = from_union([lambda x: to_class(TriggerRegion0, x), from_none], self.trigger_region1)
        if self.trigger_region2 is not None:
            result["triggerRegion2"] = from_union([lambda x: to_class(TriggerRegion0, x), from_none], self.trigger_region2)
        if self.trigger_region3 is not None:
            result["triggerRegion3"] = from_union([lambda x: to_class(TriggerRegion0, x), from_none], self.trigger_region3)
        return result


@dataclass
class ClassifierConfig:
    """Vehicle Classifier service configuration"""

    classifier: Optional[ClassifierConfigClassifier] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ClassifierConfig':
        assert isinstance(obj, dict)
        classifier = from_union([ClassifierConfigClassifier.from_dict, from_none], obj.get("classifier"))
        return ClassifierConfig(classifier)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.classifier is not None:
            result["classifier"] = from_union([lambda x: to_class(ClassifierConfigClassifier, x), from_none], self.classifier)
        return result


@dataclass
class AutoFocusRoi:
    center_x: Optional[int] = None
    center_y: Optional[int] = None
    height: Optional[int] = None
    width: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'AutoFocusRoi':
        assert isinstance(obj, dict)
        center_x = from_union([from_int, from_none], obj.get("centerX"))
        center_y = from_union([from_int, from_none], obj.get("centerY"))
        height = from_union([from_int, from_none], obj.get("height"))
        width = from_union([from_int, from_none], obj.get("width"))
        return AutoFocusRoi(center_x, center_y, height, width)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.center_x is not None:
            result["centerX"] = from_union([from_int, from_none], self.center_x)
        if self.center_y is not None:
            result["centerY"] = from_union([from_int, from_none], self.center_y)
        if self.height is not None:
            result["height"] = from_union([from_int, from_none], self.height)
        if self.width is not None:
            result["width"] = from_union([from_int, from_none], self.width)
        return result


@dataclass
class AutoFocus:
    """AutoFocus configs"""

    coarse_step: Optional[int] = None
    contrast_threshold: Optional[float] = None
    roi: Optional[AutoFocusRoi] = None
    run: Optional[bool] = None
    update_rate: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'AutoFocus':
        assert isinstance(obj, dict)
        coarse_step = from_union([from_int, from_none], obj.get("coarseStep"))
        contrast_threshold = from_union([from_float, from_none], obj.get("contrastThreshold"))
        roi = from_union([AutoFocusRoi.from_dict, from_none], obj.get("roi"))
        run = from_union([from_bool, from_none], obj.get("run"))
        update_rate = from_union([from_int, from_none], obj.get("updateRate"))
        return AutoFocus(coarse_step, contrast_threshold, roi, run, update_rate)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.coarse_step is not None:
            result["coarseStep"] = from_union([from_int, from_none], self.coarse_step)
        if self.contrast_threshold is not None:
            result["contrastThreshold"] = from_union([to_float, from_none], self.contrast_threshold)
        if self.roi is not None:
            result["roi"] = from_union([lambda x: to_class(AutoFocusRoi, x), from_none], self.roi)
        if self.run is not None:
            result["run"] = from_union([from_bool, from_none], self.run)
        if self.update_rate is not None:
            result["updateRate"] = from_union([from_int, from_none], self.update_rate)
        return result


@dataclass
class H264Main:
    """Single h26x stream configuration"""

    available: Optional[bool] = None
    bitrate: Optional[int] = None
    control_rate: Optional[str] = None
    enabled: Optional[bool] = None
    gop: Optional[int] = None
    profile: Optional[str] = None
    running: Optional[bool] = None
    source: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'H264Main':
        assert isinstance(obj, dict)
        available = from_union([from_bool, from_none], obj.get("available"))
        bitrate = from_union([from_int, from_none], obj.get("bitrate"))
        control_rate = from_union([from_str, from_none], obj.get("controlRate"))
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        gop = from_union([from_int, from_none], obj.get("gop"))
        profile = from_union([from_str, from_none], obj.get("profile"))
        running = from_union([from_bool, from_none], obj.get("running"))
        source = from_union([from_str, from_none], obj.get("source"))
        return H264Main(available, bitrate, control_rate, enabled, gop, profile, running, source)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.available is not None:
            result["available"] = from_union([from_bool, from_none], self.available)
        if self.bitrate is not None:
            result["bitrate"] = from_union([from_int, from_none], self.bitrate)
        if self.control_rate is not None:
            result["controlRate"] = from_union([from_str, from_none], self.control_rate)
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.gop is not None:
            result["gop"] = from_union([from_int, from_none], self.gop)
        if self.profile is not None:
            result["profile"] = from_union([from_str, from_none], self.profile)
        if self.running is not None:
            result["running"] = from_union([from_bool, from_none], self.running)
        if self.source is not None:
            result["source"] = from_union([from_str, from_none], self.source)
        return result


@dataclass
class H264:
    available: Optional[bool] = None
    encoder_type: Optional[str] = None
    main: Optional[H264Main] = None

    @staticmethod
    def from_dict(obj: Any) -> 'H264':
        assert isinstance(obj, dict)
        available = from_union([from_bool, from_none], obj.get("available"))
        encoder_type = from_union([from_str, from_none], obj.get("encoder_type"))
        main = from_union([H264Main.from_dict, from_none], obj.get("main"))
        return H264(available, encoder_type, main)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.available is not None:
            result["available"] = from_union([from_bool, from_none], self.available)
        if self.encoder_type is not None:
            result["encoder_type"] = from_union([from_str, from_none], self.encoder_type)
        if self.main is not None:
            result["main"] = from_union([lambda x: to_class(H264Main, x), from_none], self.main)
        return result


@dataclass
class MjpegMain:
    """Single MJPEG stream configuration"""

    available: Optional[bool] = None
    enabled: Optional[bool] = None
    framerate: Optional[float] = None
    quality: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'MjpegMain':
        assert isinstance(obj, dict)
        available = from_union([from_bool, from_none], obj.get("available"))
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        framerate = from_union([from_float, from_none], obj.get("framerate"))
        quality = from_union([from_int, from_none], obj.get("quality"))
        return MjpegMain(available, enabled, framerate, quality)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.available is not None:
            result["available"] = from_union([from_bool, from_none], self.available)
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.framerate is not None:
            result["framerate"] = from_union([to_float, from_none], self.framerate)
        if self.quality is not None:
            result["quality"] = from_union([from_int, from_none], self.quality)
        return result


@dataclass
class Mjpeg:
    available: Optional[bool] = None
    main: Optional[MjpegMain] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Mjpeg':
        assert isinstance(obj, dict)
        available = from_union([from_bool, from_none], obj.get("available"))
        main = from_union([MjpegMain.from_dict, from_none], obj.get("main"))
        return Mjpeg(available, main)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.available is not None:
            result["available"] = from_union([from_bool, from_none], self.available)
        if self.main is not None:
            result["main"] = from_union([lambda x: to_class(MjpegMain, x), from_none], self.main)
        return result


@dataclass
class StreamConfig:
    """Stream(s) configuration of device"""

    h264: Optional[H264] = None
    mjpeg: Optional[Mjpeg] = None

    @staticmethod
    def from_dict(obj: Any) -> 'StreamConfig':
        assert isinstance(obj, dict)
        h264 = from_union([H264.from_dict, from_none], obj.get("h264"))
        mjpeg = from_union([Mjpeg.from_dict, from_none], obj.get("mjpeg"))
        return StreamConfig(h264, mjpeg)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.h264 is not None:
            result["h264"] = from_union([lambda x: to_class(H264, x), from_none], self.h264)
        if self.mjpeg is not None:
            result["mjpeg"] = from_union([lambda x: to_class(Mjpeg, x), from_none], self.mjpeg)
        return result


@dataclass
class Scenario1Crop:
    x0: Optional[int] = None
    x1: Optional[int] = None
    y0: Optional[int] = None
    y1: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Scenario1Crop':
        assert isinstance(obj, dict)
        x0 = from_union([from_int, from_none], obj.get("x0"))
        x1 = from_union([from_int, from_none], obj.get("x1"))
        y0 = from_union([from_int, from_none], obj.get("y0"))
        y1 = from_union([from_int, from_none], obj.get("y1"))
        return Scenario1Crop(x0, x1, y0, y1)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.x0 is not None:
            result["x0"] = from_union([from_int, from_none], self.x0)
        if self.x1 is not None:
            result["x1"] = from_union([from_int, from_none], self.x1)
        if self.y0 is not None:
            result["y0"] = from_union([from_int, from_none], self.y0)
        if self.y1 is not None:
            result["y1"] = from_union([from_int, from_none], self.y1)
        return result


@dataclass
class Scenario2Crop:
    x0: Optional[int] = None
    x1: Optional[int] = None
    y0: Optional[int] = None
    y1: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Scenario2Crop':
        assert isinstance(obj, dict)
        x0 = from_union([from_int, from_none], obj.get("x0"))
        x1 = from_union([from_int, from_none], obj.get("x1"))
        y0 = from_union([from_int, from_none], obj.get("y0"))
        y1 = from_union([from_int, from_none], obj.get("y1"))
        return Scenario2Crop(x0, x1, y0, y1)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.x0 is not None:
            result["x0"] = from_union([from_int, from_none], self.x0)
        if self.x1 is not None:
            result["x1"] = from_union([from_int, from_none], self.x1)
        if self.y0 is not None:
            result["y0"] = from_union([from_int, from_none], self.y0)
        if self.y1 is not None:
            result["y1"] = from_union([from_int, from_none], self.y1)
        return result


@dataclass
class SnapshotCrop:
    enable: Optional[bool] = None
    mode: Optional[str] = None
    x0: Optional[int] = None
    x1: Optional[int] = None
    y0: Optional[int] = None
    y1: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'SnapshotCrop':
        assert isinstance(obj, dict)
        enable = from_union([from_bool, from_none], obj.get("enable"))
        mode = from_union([from_str, from_none], obj.get("mode"))
        x0 = from_union([from_int, from_none], obj.get("x0"))
        x1 = from_union([from_int, from_none], obj.get("x1"))
        y0 = from_union([from_int, from_none], obj.get("y0"))
        y1 = from_union([from_int, from_none], obj.get("y1"))
        return SnapshotCrop(enable, mode, x0, x1, y0, y1)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.enable is not None:
            result["enable"] = from_union([from_bool, from_none], self.enable)
        if self.mode is not None:
            result["mode"] = from_union([from_str, from_none], self.mode)
        if self.x0 is not None:
            result["x0"] = from_union([from_int, from_none], self.x0)
        if self.x1 is not None:
            result["x1"] = from_union([from_int, from_none], self.x1)
        if self.y0 is not None:
            result["y0"] = from_union([from_int, from_none], self.y0)
        if self.y1 is not None:
            result["y1"] = from_union([from_int, from_none], self.y1)
        return result


@dataclass
class Misc:
    """Miscellaneous configs"""

    camera_orientation: Optional[bool] = None
    iris_hint: Optional[str] = None
    jpeg_quality: Optional[int] = None
    legacy_tsync_gpio: Optional[int] = None
    scenario1_crop: Optional[Scenario1Crop] = None
    scenario1_overlay: Optional[str] = None
    scenario1_overlay_text_size: Optional[int] = None
    scenario2_crop: Optional[Scenario2Crop] = None
    scenario2_overlay: Optional[str] = None
    scenario2_overlay_text_size: Optional[int] = None
    scenario_overlay_color: Optional[str] = None
    snapshot_crop: Optional[SnapshotCrop] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Misc':
        assert isinstance(obj, dict)
        camera_orientation = from_union([from_bool, from_none], obj.get("cameraOrientation"))
        iris_hint = from_union([from_str, from_none], obj.get("irisHint"))
        jpeg_quality = from_union([from_int, from_none], obj.get("jpegQuality"))
        legacy_tsync_gpio = from_union([from_int, from_none], obj.get("legacyTsyncGpio"))
        scenario1_crop = from_union([Scenario1Crop.from_dict, from_none], obj.get("scenario1Crop"))
        scenario1_overlay = from_union([from_str, from_none], obj.get("scenario1Overlay"))
        scenario1_overlay_text_size = from_union([from_int, from_none], obj.get("scenario1OverlayTextSize"))
        scenario2_crop = from_union([Scenario2Crop.from_dict, from_none], obj.get("scenario2Crop"))
        scenario2_overlay = from_union([from_str, from_none], obj.get("scenario2Overlay"))
        scenario2_overlay_text_size = from_union([from_int, from_none], obj.get("scenario2OverlayTextSize"))
        scenario_overlay_color = from_union([from_str, from_none], obj.get("scenarioOverlayColor"))
        snapshot_crop = from_union([SnapshotCrop.from_dict, from_none], obj.get("snapshotCrop"))
        return Misc(camera_orientation, iris_hint, jpeg_quality, legacy_tsync_gpio, scenario1_crop, scenario1_overlay, scenario1_overlay_text_size, scenario2_crop, scenario2_overlay, scenario2_overlay_text_size, scenario_overlay_color, snapshot_crop)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.camera_orientation is not None:
            result["cameraOrientation"] = from_union([from_bool, from_none], self.camera_orientation)
        if self.iris_hint is not None:
            result["irisHint"] = from_union([from_str, from_none], self.iris_hint)
        if self.jpeg_quality is not None:
            result["jpegQuality"] = from_union([from_int, from_none], self.jpeg_quality)
        if self.legacy_tsync_gpio is not None:
            result["legacyTsyncGpio"] = from_union([from_int, from_none], self.legacy_tsync_gpio)
        if self.scenario1_crop is not None:
            result["scenario1Crop"] = from_union([lambda x: to_class(Scenario1Crop, x), from_none], self.scenario1_crop)
        if self.scenario1_overlay is not None:
            result["scenario1Overlay"] = from_union([from_str, from_none], self.scenario1_overlay)
        if self.scenario1_overlay_text_size is not None:
            result["scenario1OverlayTextSize"] = from_union([from_int, from_none], self.scenario1_overlay_text_size)
        if self.scenario2_crop is not None:
            result["scenario2Crop"] = from_union([lambda x: to_class(Scenario2Crop, x), from_none], self.scenario2_crop)
        if self.scenario2_overlay is not None:
            result["scenario2Overlay"] = from_union([from_str, from_none], self.scenario2_overlay)
        if self.scenario2_overlay_text_size is not None:
            result["scenario2OverlayTextSize"] = from_union([from_int, from_none], self.scenario2_overlay_text_size)
        if self.scenario_overlay_color is not None:
            result["scenarioOverlayColor"] = from_union([from_str, from_none], self.scenario_overlay_color)
        if self.snapshot_crop is not None:
            result["snapshotCrop"] = from_union([lambda x: to_class(SnapshotCrop, x), from_none], self.snapshot_crop)
        return result


@dataclass
class AE:
    ctrl_mode: Optional[str] = None
    last_run: Optional[int] = None
    level: Optional[float] = None

    @staticmethod
    def from_dict(obj: Any) -> 'AE':
        assert isinstance(obj, dict)
        ctrl_mode = from_union([from_str, from_none], obj.get("ctrlMode"))
        last_run = from_union([from_int, from_none], obj.get("lastRun"))
        level = from_union([from_float, from_none], obj.get("level"))
        return AE(ctrl_mode, last_run, level)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.ctrl_mode is not None:
            result["ctrlMode"] = from_union([from_str, from_none], self.ctrl_mode)
        if self.last_run is not None:
            result["lastRun"] = from_union([from_int, from_none], self.last_run)
        if self.level is not None:
            result["level"] = from_union([to_float, from_none], self.level)
        return result


@dataclass
class FPS:
    mjpeg: Optional[float] = None

    @staticmethod
    def from_dict(obj: Any) -> 'FPS':
        assert isinstance(obj, dict)
        mjpeg = from_union([from_float, from_none], obj.get("mjpeg"))
        return FPS(mjpeg)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.mjpeg is not None:
            result["mjpeg"] = from_union([to_float, from_none], self.mjpeg)
        return result


@dataclass
class Gps:
    altitude: Optional[float] = None
    available: Optional[bool] = None
    bearing: Optional[float] = None
    dop: Optional[float] = None
    fix: Optional[str] = None
    latitude: Optional[float] = None
    longitude: Optional[float] = None
    num_satellites: Optional[int] = None
    seconds_since_last_fix: Optional[int] = None
    speed: Optional[float] = None
    time: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Gps':
        assert isinstance(obj, dict)
        altitude = from_union([from_float, from_none], obj.get("altitude"))
        available = from_union([from_bool, from_none], obj.get("available"))
        bearing = from_union([from_float, from_none], obj.get("bearing"))
        dop = from_union([from_float, from_none], obj.get("dop"))
        fix = from_union([from_str, from_none], obj.get("fix"))
        latitude = from_union([from_float, from_none], obj.get("latitude"))
        longitude = from_union([from_float, from_none], obj.get("longitude"))
        num_satellites = from_union([from_int, from_none], obj.get("numSatellites"))
        seconds_since_last_fix = from_union([from_int, from_none], obj.get("secondsSinceLastFix"))
        speed = from_union([from_float, from_none], obj.get("speed"))
        time = from_union([from_int, from_none], obj.get("time"))
        return Gps(altitude, available, bearing, dop, fix, latitude, longitude, num_satellites, seconds_since_last_fix, speed, time)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.altitude is not None:
            result["altitude"] = from_union([to_float, from_none], self.altitude)
        if self.available is not None:
            result["available"] = from_union([from_bool, from_none], self.available)
        if self.bearing is not None:
            result["bearing"] = from_union([to_float, from_none], self.bearing)
        if self.dop is not None:
            result["dop"] = from_union([to_float, from_none], self.dop)
        if self.fix is not None:
            result["fix"] = from_union([from_str, from_none], self.fix)
        if self.latitude is not None:
            result["latitude"] = from_union([to_float, from_none], self.latitude)
        if self.longitude is not None:
            result["longitude"] = from_union([to_float, from_none], self.longitude)
        if self.num_satellites is not None:
            result["numSatellites"] = from_union([from_int, from_none], self.num_satellites)
        if self.seconds_since_last_fix is not None:
            result["secondsSinceLastFix"] = from_union([from_int, from_none], self.seconds_since_last_fix)
        if self.speed is not None:
            result["speed"] = from_union([to_float, from_none], self.speed)
        if self.time is not None:
            result["time"] = from_union([from_int, from_none], self.time)
        return result


@dataclass
class ISP:
    free_buffers: Optional[int] = None
    gain: Optional[int] = None
    iris: Optional[int] = None
    iris_model: Optional[str] = None
    shutter: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ISP':
        assert isinstance(obj, dict)
        free_buffers = from_union([from_int, from_none], obj.get("freeBuffers"))
        gain = from_union([from_int, from_none], obj.get("gain"))
        iris = from_union([from_int, from_none], obj.get("iris"))
        iris_model = from_union([from_str, from_none], obj.get("irisModel"))
        shutter = from_union([from_int, from_none], obj.get("shutter"))
        return ISP(free_buffers, gain, iris, iris_model, shutter)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.free_buffers is not None:
            result["freeBuffers"] = from_union([from_int, from_none], self.free_buffers)
        if self.gain is not None:
            result["gain"] = from_union([from_int, from_none], self.gain)
        if self.iris is not None:
            result["iris"] = from_union([from_int, from_none], self.iris)
        if self.iris_model is not None:
            result["irisModel"] = from_union([from_str, from_none], self.iris_model)
        if self.shutter is not None:
            result["shutter"] = from_union([from_int, from_none], self.shutter)
        return result


@dataclass
class MiscVolatileLens:
    focus: Optional[int] = None
    zoom: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'MiscVolatileLens':
        assert isinstance(obj, dict)
        focus = from_union([from_int, from_none], obj.get("focus"))
        zoom = from_union([from_int, from_none], obj.get("zoom"))
        return MiscVolatileLens(focus, zoom)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.focus is not None:
            result["focus"] = from_union([from_int, from_none], self.focus)
        if self.zoom is not None:
            result["zoom"] = from_union([from_int, from_none], self.zoom)
        return result


@dataclass
class Profile:
    id: Optional[int] = None
    name: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Profile':
        assert isinstance(obj, dict)
        id = from_union([from_int, from_none], obj.get("id"))
        name = from_union([from_str, from_none], obj.get("name"))
        return Profile(id, name)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.id is not None:
            result["id"] = from_union([from_int, from_none], self.id)
        if self.name is not None:
            result["name"] = from_union([from_str, from_none], self.name)
        return result


@dataclass
class MiscVolatile:
    """Current Miscellaneous Read-Only configs"""

    ae: Optional[AE] = None
    fps: Optional[FPS] = None
    gps: Optional[Gps] = None
    isp: Optional[ISP] = None
    lens: Optional[MiscVolatileLens] = None
    profile: Optional[Profile] = None
    whitebalance: Optional[WhitebalanceClass] = None

    @staticmethod
    def from_dict(obj: Any) -> 'MiscVolatile':
        assert isinstance(obj, dict)
        ae = from_union([AE.from_dict, from_none], obj.get("ae"))
        fps = from_union([FPS.from_dict, from_none], obj.get("fps"))
        gps = from_union([Gps.from_dict, from_none], obj.get("gps"))
        isp = from_union([ISP.from_dict, from_none], obj.get("isp"))
        lens = from_union([MiscVolatileLens.from_dict, from_none], obj.get("lens"))
        profile = from_union([Profile.from_dict, from_none], obj.get("profile"))
        whitebalance = from_union([WhitebalanceClass.from_dict, from_none], obj.get("whitebalance"))
        return MiscVolatile(ae, fps, gps, isp, lens, profile, whitebalance)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.ae is not None:
            result["ae"] = from_union([lambda x: to_class(AE, x), from_none], self.ae)
        if self.fps is not None:
            result["fps"] = from_union([lambda x: to_class(FPS, x), from_none], self.fps)
        if self.gps is not None:
            result["gps"] = from_union([lambda x: to_class(Gps, x), from_none], self.gps)
        if self.isp is not None:
            result["isp"] = from_union([lambda x: to_class(ISP, x), from_none], self.isp)
        if self.lens is not None:
            result["lens"] = from_union([lambda x: to_class(MiscVolatileLens, x), from_none], self.lens)
        if self.profile is not None:
            result["profile"] = from_union([lambda x: to_class(Profile, x), from_none], self.profile)
        if self.whitebalance is not None:
            result["whitebalance"] = from_union([lambda x: to_class(WhitebalanceClass, x), from_none], self.whitebalance)
        return result


@dataclass
class Itscampro:
    address: Optional[str] = None
    debug: Optional[bool] = None
    enable: Optional[bool] = None
    port: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Itscampro':
        assert isinstance(obj, dict)
        address = from_union([from_str, from_none], obj.get("address"))
        debug = from_union([from_bool, from_none], obj.get("debug"))
        enable = from_union([from_bool, from_none], obj.get("enable"))
        port = from_union([from_int, from_none], obj.get("port"))
        return Itscampro(address, debug, enable, port)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.address is not None:
            result["address"] = from_union([from_str, from_none], self.address)
        if self.debug is not None:
            result["debug"] = from_union([from_bool, from_none], self.debug)
        if self.enable is not None:
            result["enable"] = from_union([from_bool, from_none], self.enable)
        if self.port is not None:
            result["port"] = from_union([from_int, from_none], self.port)
        return result


@dataclass
class ItscamproConfig:
    """ITSCAMPRO service configuration"""

    itscampro: Optional[Itscampro] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ItscamproConfig':
        assert isinstance(obj, dict)
        itscampro = from_union([Itscampro.from_dict, from_none], obj.get("itscampro"))
        return ItscamproConfig(itscampro)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.itscampro is not None:
            result["itscampro"] = from_union([lambda x: to_class(Itscampro, x), from_none], self.itscampro)
        return result


@dataclass
class ItscamproStatus:
    """ITSCAMPRO service status"""

    status: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ItscamproStatus':
        assert isinstance(obj, dict)
        status = from_union([from_str, from_none], obj.get("status"))
        return ItscamproStatus(status)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.status is not None:
            result["status"] = from_union([from_str, from_none], self.status)
        return result


@dataclass
class Sign:
    append_mode: Optional[str] = None
    enabled: Optional[bool] = None
    loaded: Optional[bool] = None
    update: Optional[bool] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Sign':
        assert isinstance(obj, dict)
        append_mode = from_union([from_str, from_none], obj.get("appendMode"))
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        loaded = from_union([from_bool, from_none], obj.get("loaded"))
        update = from_union([from_bool, from_none], obj.get("update"))
        return Sign(append_mode, enabled, loaded, update)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.append_mode is not None:
            result["appendMode"] = from_union([from_str, from_none], self.append_mode)
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.loaded is not None:
            result["loaded"] = from_union([from_bool, from_none], self.loaded)
        if self.update is not None:
            result["update"] = from_union([from_bool, from_none], self.update)
        return result


@dataclass
class ImageSignConfig:
    """ImageSign service configuration"""

    sign: Optional[Sign] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ImageSignConfig':
        assert isinstance(obj, dict)
        sign = from_union([Sign.from_dict, from_none], obj.get("sign"))
        return ImageSignConfig(sign)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.sign is not None:
            result["sign"] = from_union([lambda x: to_class(Sign, x), from_none], self.sign)
        return result


@dataclass
class Local:
    buffer_size_kb: Optional[int] = None
    ttl: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Local':
        assert isinstance(obj, dict)
        buffer_size_kb = from_union([from_int, from_none], obj.get("bufferSizeKb"))
        ttl = from_union([from_int, from_none], obj.get("ttl"))
        return Local(buffer_size_kb, ttl)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.buffer_size_kb is not None:
            result["bufferSizeKb"] = from_union([from_int, from_none], self.buffer_size_kb)
        if self.ttl is not None:
            result["ttl"] = from_union([from_int, from_none], self.ttl)
        return result


@dataclass
class Transfer:
    poll_interval: Optional[int] = None
    timeout: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Transfer':
        assert isinstance(obj, dict)
        poll_interval = from_union([from_int, from_none], obj.get("pollInterval"))
        timeout = from_union([from_int, from_none], obj.get("timeout"))
        return Transfer(poll_interval, timeout)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.poll_interval is not None:
            result["pollInterval"] = from_union([from_int, from_none], self.poll_interval)
        if self.timeout is not None:
            result["timeout"] = from_union([from_int, from_none], self.timeout)
        return result


@dataclass
class FTP:
    address: Optional[str] = None
    anonymous: Optional[bool] = None
    enable: Optional[bool] = None
    filename: Optional[str] = None
    local: Optional[Local] = None
    password: Optional[str] = None
    port: Optional[int] = None
    protocol: Optional[str] = None
    quality: Optional[int] = None
    transfer: Optional[Transfer] = None
    username: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'FTP':
        assert isinstance(obj, dict)
        address = from_union([from_str, from_none], obj.get("address"))
        anonymous = from_union([from_bool, from_none], obj.get("anonymous"))
        enable = from_union([from_bool, from_none], obj.get("enable"))
        filename = from_union([from_str, from_none], obj.get("filename"))
        local = from_union([Local.from_dict, from_none], obj.get("local"))
        password = from_union([from_str, from_none], obj.get("password"))
        port = from_union([from_int, from_none], obj.get("port"))
        protocol = from_union([from_str, from_none], obj.get("protocol"))
        quality = from_union([from_int, from_none], obj.get("quality"))
        transfer = from_union([Transfer.from_dict, from_none], obj.get("transfer"))
        username = from_union([from_str, from_none], obj.get("username"))
        return FTP(address, anonymous, enable, filename, local, password, port, protocol, quality, transfer, username)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.address is not None:
            result["address"] = from_union([from_str, from_none], self.address)
        if self.anonymous is not None:
            result["anonymous"] = from_union([from_bool, from_none], self.anonymous)
        if self.enable is not None:
            result["enable"] = from_union([from_bool, from_none], self.enable)
        if self.filename is not None:
            result["filename"] = from_union([from_str, from_none], self.filename)
        if self.local is not None:
            result["local"] = from_union([lambda x: to_class(Local, x), from_none], self.local)
        if self.password is not None:
            result["password"] = from_union([from_str, from_none], self.password)
        if self.port is not None:
            result["port"] = from_union([from_int, from_none], self.port)
        if self.protocol is not None:
            result["protocol"] = from_union([from_str, from_none], self.protocol)
        if self.quality is not None:
            result["quality"] = from_union([from_int, from_none], self.quality)
        if self.transfer is not None:
            result["transfer"] = from_union([lambda x: to_class(Transfer, x), from_none], self.transfer)
        if self.username is not None:
            result["username"] = from_union([from_str, from_none], self.username)
        return result


@dataclass
class FTPConfig:
    """FTP service configuration"""

    ftp: Optional[FTP] = None

    @staticmethod
    def from_dict(obj: Any) -> 'FTPConfig':
        assert isinstance(obj, dict)
        ftp = from_union([FTP.from_dict, from_none], obj.get("ftp"))
        return FTPConfig(ftp)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.ftp is not None:
            result["ftp"] = from_union([lambda x: to_class(FTP, x), from_none], self.ftp)
        return result


@dataclass
class LinceConfig:
    """Lince service configuration"""

    auth_code: Optional[str] = None
    client_endpoint: Optional[str] = None
    client_id: Optional[str] = None
    enabled: Optional[bool] = None
    environment: Optional[str] = None
    send_recs_none: Optional[bool] = None
    timeout_response: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'LinceConfig':
        assert isinstance(obj, dict)
        auth_code = from_union([from_str, from_none], obj.get("authCode"))
        client_endpoint = from_union([from_str, from_none], obj.get("clientEndpoint"))
        client_id = from_union([from_str, from_none], obj.get("clientId"))
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        environment = from_union([from_str, from_none], obj.get("environment"))
        send_recs_none = from_union([from_bool, from_none], obj.get("sendRecsNone"))
        timeout_response = from_union([from_int, from_none], obj.get("timeoutResponse"))
        return LinceConfig(auth_code, client_endpoint, client_id, enabled, environment, send_recs_none, timeout_response)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.auth_code is not None:
            result["authCode"] = from_union([from_str, from_none], self.auth_code)
        if self.client_endpoint is not None:
            result["clientEndpoint"] = from_union([from_str, from_none], self.client_endpoint)
        if self.client_id is not None:
            result["clientId"] = from_union([from_str, from_none], self.client_id)
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.environment is not None:
            result["environment"] = from_union([from_str, from_none], self.environment)
        if self.send_recs_none is not None:
            result["sendRecsNone"] = from_union([from_bool, from_none], self.send_recs_none)
        if self.timeout_response is not None:
            result["timeoutResponse"] = from_union([from_int, from_none], self.timeout_response)
        return result


@dataclass
class LinceStatus:
    """Lince service status"""

    lince_status: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'LinceStatus':
        assert isinstance(obj, dict)
        lince_status = from_union([from_str, from_none], obj.get("linceStatus"))
        return LinceStatus(lince_status)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.lince_status is not None:
            result["linceStatus"] = from_union([from_str, from_none], self.lince_status)
        return result


@dataclass
class VehicleIndicator:
    vehicle_counter_active_high: Optional[bool] = None
    vehicle_counter_enabled: Optional[bool] = None
    vehicle_counter_gpio: Optional[int] = None
    vehicle_counter_pulse_width_ms: Optional[int] = None
    vehicle_counter_type: Optional[int] = None
    vehicle_counter_udp_port: Optional[int] = None
    vehicle_counter_udp_sample_time_ms: Optional[int] = None
    vehicle_counter_udp_server: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'VehicleIndicator':
        assert isinstance(obj, dict)
        vehicle_counter_active_high = from_union([from_bool, from_none], obj.get("vehicleCounterActiveHigh"))
        vehicle_counter_enabled = from_union([from_bool, from_none], obj.get("vehicleCounterEnabled"))
        vehicle_counter_gpio = from_union([from_int, from_none], obj.get("vehicleCounterGpio"))
        vehicle_counter_pulse_width_ms = from_union([from_int, from_none], obj.get("vehicleCounterPulseWidthMs"))
        vehicle_counter_type = from_union([from_int, from_none], obj.get("vehicleCounterType"))
        vehicle_counter_udp_port = from_union([from_int, from_none], obj.get("vehicleCounterUdpPort"))
        vehicle_counter_udp_sample_time_ms = from_union([from_int, from_none], obj.get("vehicleCounterUdpSampleTimeMs"))
        vehicle_counter_udp_server = from_union([from_str, from_none], obj.get("vehicleCounterUdpServer"))
        return VehicleIndicator(vehicle_counter_active_high, vehicle_counter_enabled, vehicle_counter_gpio, vehicle_counter_pulse_width_ms, vehicle_counter_type, vehicle_counter_udp_port, vehicle_counter_udp_sample_time_ms, vehicle_counter_udp_server)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.vehicle_counter_active_high is not None:
            result["vehicleCounterActiveHigh"] = from_union([from_bool, from_none], self.vehicle_counter_active_high)
        if self.vehicle_counter_enabled is not None:
            result["vehicleCounterEnabled"] = from_union([from_bool, from_none], self.vehicle_counter_enabled)
        if self.vehicle_counter_gpio is not None:
            result["vehicleCounterGpio"] = from_union([from_int, from_none], self.vehicle_counter_gpio)
        if self.vehicle_counter_pulse_width_ms is not None:
            result["vehicleCounterPulseWidthMs"] = from_union([from_int, from_none], self.vehicle_counter_pulse_width_ms)
        if self.vehicle_counter_type is not None:
            result["vehicleCounterType"] = from_union([from_int, from_none], self.vehicle_counter_type)
        if self.vehicle_counter_udp_port is not None:
            result["vehicleCounterUdpPort"] = from_union([from_int, from_none], self.vehicle_counter_udp_port)
        if self.vehicle_counter_udp_sample_time_ms is not None:
            result["vehicleCounterUdpSampleTimeMs"] = from_union([from_int, from_none], self.vehicle_counter_udp_sample_time_ms)
        if self.vehicle_counter_udp_server is not None:
            result["vehicleCounterUdpServer"] = from_union([from_str, from_none], self.vehicle_counter_udp_server)
        return result


@dataclass
class VehicleIndicatorConfig:
    """VehicleIndicator service configuration"""

    vehicle_indicator: Optional[VehicleIndicator] = None

    @staticmethod
    def from_dict(obj: Any) -> 'VehicleIndicatorConfig':
        assert isinstance(obj, dict)
        vehicle_indicator = from_union([VehicleIndicator.from_dict, from_none], obj.get("vehicleIndicator"))
        return VehicleIndicatorConfig(vehicle_indicator)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.vehicle_indicator is not None:
            result["vehicleIndicator"] = from_union([lambda x: to_class(VehicleIndicator, x), from_none], self.vehicle_indicator)
        return result


@dataclass
class ConfigCGI:
    block_api: Optional[bool] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ConfigCGI':
        assert isinstance(obj, dict)
        block_api = from_union([from_bool, from_none], obj.get("blockAPI"))
        return ConfigCGI(block_api)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.block_api is not None:
            result["blockAPI"] = from_union([from_bool, from_none], self.block_api)
        return result


@dataclass
class Auth:
    password: Optional[str] = None
    require: Optional[bool] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Auth':
        assert isinstance(obj, dict)
        password = from_union([from_str, from_none], obj.get("password"))
        require = from_union([from_bool, from_none], obj.get("require"))
        return Auth(password, require)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.password is not None:
            result["password"] = from_union([from_str, from_none], self.password)
        if self.require is not None:
            result["require"] = from_union([from_bool, from_none], self.require)
        return result


@dataclass
class Cougar:
    auth: Optional[Auth] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Cougar':
        assert isinstance(obj, dict)
        auth = from_union([Auth.from_dict, from_none], obj.get("auth"))
        return Cougar(auth)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.auth is not None:
            result["auth"] = from_union([lambda x: to_class(Auth, x), from_none], self.auth)
        return result


@dataclass
class Itscamprotocol:
    legacy_mode: Optional[bool] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Itscamprotocol':
        assert isinstance(obj, dict)
        legacy_mode = from_union([from_bool, from_none], obj.get("legacyMode"))
        return Itscamprotocol(legacy_mode)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.legacy_mode is not None:
            result["legacyMode"] = from_union([from_bool, from_none], self.legacy_mode)
        return result


@dataclass
class ProtocolsConfig:
    """Protocols configurations"""

    config_cgi: Optional[ConfigCGI] = None
    cougar: Optional[Cougar] = None
    itscamprotocol: Optional[Itscamprotocol] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ProtocolsConfig':
        assert isinstance(obj, dict)
        config_cgi = from_union([ConfigCGI.from_dict, from_none], obj.get("configCgi"))
        cougar = from_union([Cougar.from_dict, from_none], obj.get("cougar"))
        itscamprotocol = from_union([Itscamprotocol.from_dict, from_none], obj.get("itscamprotocol"))
        return ProtocolsConfig(config_cgi, cougar, itscamprotocol)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.config_cgi is not None:
            result["configCgi"] = from_union([lambda x: to_class(ConfigCGI, x), from_none], self.config_cgi)
        if self.cougar is not None:
            result["cougar"] = from_union([lambda x: to_class(Cougar, x), from_none], self.cougar)
        if self.itscamprotocol is not None:
            result["itscamprotocol"] = from_union([lambda x: to_class(Itscamprotocol, x), from_none], self.itscamprotocol)
        return result


@dataclass
class ProfileTransitioner:
    """Profile Transitioner configs"""

    automatic: Optional[bool] = None
    level_smoothing: Optional[str] = None
    reset_profiles: Optional[str] = None
    smoothing_time: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'ProfileTransitioner':
        assert isinstance(obj, dict)
        automatic = from_union([from_bool, from_none], obj.get("automatic"))
        level_smoothing = from_union([from_str, from_none], obj.get("levelSmoothing"))
        reset_profiles = from_union([from_str, from_none], obj.get("resetProfiles"))
        smoothing_time = from_union([from_int, from_none], obj.get("smoothingTime"))
        return ProfileTransitioner(automatic, level_smoothing, reset_profiles, smoothing_time)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.automatic is not None:
            result["automatic"] = from_union([from_bool, from_none], self.automatic)
        if self.level_smoothing is not None:
            result["levelSmoothing"] = from_union([from_str, from_none], self.level_smoothing)
        if self.reset_profiles is not None:
            result["resetProfiles"] = from_union([from_str, from_none], self.reset_profiles)
        if self.smoothing_time is not None:
            result["smoothingTime"] = from_union([from_int, from_none], self.smoothing_time)
        return result


@dataclass
class Region0:
    """Lane region"""

    name: Optional[str] = None
    x0: Optional[int] = None
    x1: Optional[int] = None
    x2: Optional[int] = None
    x3: Optional[int] = None
    y0: Optional[int] = None
    y1: Optional[int] = None
    y2: Optional[int] = None
    y3: Optional[int] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Region0':
        assert isinstance(obj, dict)
        name = from_union([from_str, from_none], obj.get("name"))
        x0 = from_union([from_int, from_none], obj.get("x0"))
        x1 = from_union([from_int, from_none], obj.get("x1"))
        x2 = from_union([from_int, from_none], obj.get("x2"))
        x3 = from_union([from_int, from_none], obj.get("x3"))
        y0 = from_union([from_int, from_none], obj.get("y0"))
        y1 = from_union([from_int, from_none], obj.get("y1"))
        y2 = from_union([from_int, from_none], obj.get("y2"))
        y3 = from_union([from_int, from_none], obj.get("y3"))
        return Region0(name, x0, x1, x2, x3, y0, y1, y2, y3)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.name is not None:
            result["name"] = from_union([from_str, from_none], self.name)
        if self.x0 is not None:
            result["x0"] = from_union([from_int, from_none], self.x0)
        if self.x1 is not None:
            result["x1"] = from_union([from_int, from_none], self.x1)
        if self.x2 is not None:
            result["x2"] = from_union([from_int, from_none], self.x2)
        if self.x3 is not None:
            result["x3"] = from_union([from_int, from_none], self.x3)
        if self.y0 is not None:
            result["y0"] = from_union([from_int, from_none], self.y0)
        if self.y1 is not None:
            result["y1"] = from_union([from_int, from_none], self.y1)
        if self.y2 is not None:
            result["y2"] = from_union([from_int, from_none], self.y2)
        if self.y3 is not None:
            result["y3"] = from_union([from_int, from_none], self.y3)
        return result


@dataclass
class LanesConfig:
    """Lanes configuration"""

    enabled: Optional[bool] = None
    region0: Optional[Region0] = None
    region1: Optional[Region0] = None
    region2: Optional[Region0] = None

    @staticmethod
    def from_dict(obj: Any) -> 'LanesConfig':
        assert isinstance(obj, dict)
        enabled = from_union([from_bool, from_none], obj.get("enabled"))
        region0 = from_union([Region0.from_dict, from_none], obj.get("region0"))
        region1 = from_union([Region0.from_dict, from_none], obj.get("region1"))
        region2 = from_union([Region0.from_dict, from_none], obj.get("region2"))
        return LanesConfig(enabled, region0, region1, region2)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.enabled is not None:
            result["enabled"] = from_union([from_bool, from_none], self.enabled)
        if self.region0 is not None:
            result["region0"] = from_union([lambda x: to_class(Region0, x), from_none], self.region0)
        if self.region1 is not None:
            result["region1"] = from_union([lambda x: to_class(Region0, x), from_none], self.region1)
        if self.region2 is not None:
            result["region2"] = from_union([lambda x: to_class(Region0, x), from_none], self.region2)
        return result


@dataclass
class IoConfig:
    """Configuration for a specific IO"""

    port: int
    can_flash: Optional[bool] = None
    can_trigger: Optional[bool] = None
    early_us: Optional[int] = None
    group: Optional[str] = None
    identifier: Optional[str] = None
    is_input: Optional[bool] = None
    is_on: Optional[bool] = None
    protection: Optional[str] = None
    type: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'IoConfig':
        assert isinstance(obj, dict)
        port = from_int(obj.get("port"))
        can_flash = from_union([from_bool, from_none], obj.get("canFlash"))
        can_trigger = from_union([from_bool, from_none], obj.get("canTrigger"))
        early_us = from_union([from_int, from_none], obj.get("earlyUs"))
        group = from_union([from_str, from_none], obj.get("group"))
        identifier = from_union([from_str, from_none], obj.get("identifier"))
        is_input = from_union([from_bool, from_none], obj.get("isInput"))
        is_on = from_union([from_bool, from_none], obj.get("isOn"))
        protection = from_union([from_str, from_none], obj.get("protection"))
        type = from_union([from_str, from_none], obj.get("type"))
        return IoConfig(port, can_flash, can_trigger, early_us, group, identifier, is_input, is_on, protection, type)

    def to_dict(self) -> dict:
        result: dict = {}
        result["port"] = from_int(self.port)
        if self.can_flash is not None:
            result["canFlash"] = from_union([from_bool, from_none], self.can_flash)
        if self.can_trigger is not None:
            result["canTrigger"] = from_union([from_bool, from_none], self.can_trigger)
        if self.early_us is not None:
            result["earlyUs"] = from_union([from_int, from_none], self.early_us)
        if self.group is not None:
            result["group"] = from_union([from_str, from_none], self.group)
        if self.identifier is not None:
            result["identifier"] = from_union([from_str, from_none], self.identifier)
        if self.is_input is not None:
            result["isInput"] = from_union([from_bool, from_none], self.is_input)
        if self.is_on is not None:
            result["isOn"] = from_union([from_bool, from_none], self.is_on)
        if self.protection is not None:
            result["protection"] = from_union([from_str, from_none], self.protection)
        if self.type is not None:
            result["type"] = from_union([from_str, from_none], self.type)
        return result


@dataclass
class IoBasic:
    """Simplified information for a specific IO"""

    port: int
    is_input: Optional[bool] = None
    is_on: Optional[bool] = None

    @staticmethod
    def from_dict(obj: Any) -> 'IoBasic':
        assert isinstance(obj, dict)
        port = from_int(obj.get("port"))
        is_input = from_union([from_bool, from_none], obj.get("isInput"))
        is_on = from_union([from_bool, from_none], obj.get("isOn"))
        return IoBasic(port, is_input, is_on)

    def to_dict(self) -> dict:
        result: dict = {}
        result["port"] = from_int(self.port)
        if self.is_input is not None:
            result["isInput"] = from_union([from_bool, from_none], self.is_input)
        if self.is_on is not None:
            result["isOn"] = from_union([from_bool, from_none], self.is_on)
        return result


class TypeEnum(Enum):
    JSON = "json"


@dataclass
class Part:
    data: str
    name: str
    type: TypeEnum

    @staticmethod
    def from_dict(obj: Any) -> 'Part':
        assert isinstance(obj, dict)
        data = from_str(obj.get("data"))
        name = from_str(obj.get("name"))
        type = TypeEnum(obj.get("type"))
        return Part(data, name, type)

    def to_dict(self) -> dict:
        result: dict = {}
        result["data"] = from_str(self.data)
        result["name"] = from_str(self.name)
        result["type"] = to_enum(TypeEnum, self.type)
        return result


class Variant(Enum):
    MULTIPART = "multipart"
    SINGLEPART = "singlepart"


@dataclass
class Body:
    parts: List[Part]
    variant: Variant

    @staticmethod
    def from_dict(obj: Any) -> 'Body':
        assert isinstance(obj, dict)
        parts = from_list(Part.from_dict, obj.get("parts"))
        variant = Variant(obj.get("variant"))
        return Body(parts, variant)

    def to_dict(self) -> dict:
        result: dict = {}
        result["parts"] = from_list(lambda x: to_class(Part, x), self.parts)
        result["variant"] = to_enum(Variant, self.variant)
        return result


@dataclass
class Header:
    name: str
    value: str

    @staticmethod
    def from_dict(obj: Any) -> 'Header':
        assert isinstance(obj, dict)
        name = from_str(obj.get("name"))
        value = from_str(obj.get("value"))
        return Header(name, value)

    def to_dict(self) -> dict:
        result: dict = {}
        result["name"] = from_str(self.name)
        result["value"] = from_str(self.value)
        return result


@dataclass
class Resolution:
    height: int
    width: int

    @staticmethod
    def from_dict(obj: Any) -> 'Resolution':
        assert isinstance(obj, dict)
        height = from_int(obj.get("height"))
        width = from_int(obj.get("width"))
        return Resolution(height, width)

    def to_dict(self) -> dict:
        result: dict = {}
        result["height"] = from_int(self.height)
        result["width"] = from_int(self.width)
        return result


@dataclass
class JPEG:
    quality: int
    resolution: Resolution

    @staticmethod
    def from_dict(obj: Any) -> 'JPEG':
        assert isinstance(obj, dict)
        quality = from_int(obj.get("quality"))
        resolution = Resolution.from_dict(obj.get("resolution"))
        return JPEG(quality, resolution)

    def to_dict(self) -> dict:
        result: dict = {}
        result["quality"] = from_int(self.quality)
        result["resolution"] = to_class(Resolution, self.resolution)
        return result


class Method(Enum):
    GET = "get"
    POST = "post"
    PUT = "put"


@dataclass
class Persistency:
    enabled: bool
    max_disk_usage: int
    max_file_age: int
    newest_first: bool

    @staticmethod
    def from_dict(obj: Any) -> 'Persistency':
        assert isinstance(obj, dict)
        enabled = from_bool(obj.get("enabled"))
        max_disk_usage = from_int(obj.get("maxDiskUsage"))
        max_file_age = from_int(obj.get("maxFileAge"))
        newest_first = from_bool(obj.get("newestFirst"))
        return Persistency(enabled, max_disk_usage, max_file_age, newest_first)

    def to_dict(self) -> dict:
        result: dict = {}
        result["enabled"] = from_bool(self.enabled)
        result["maxDiskUsage"] = from_int(self.max_disk_usage)
        result["maxFileAge"] = from_int(self.max_file_age)
        result["newestFirst"] = from_bool(self.newest_first)
        return result


@dataclass
class TLS:
    insecure: bool
    mtls_key: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'TLS':
        assert isinstance(obj, dict)
        insecure = from_bool(obj.get("insecure"))
        mtls_key = from_union([from_str, from_none], obj.get("mtlsKey"))
        return TLS(insecure, mtls_key)

    def to_dict(self) -> dict:
        result: dict = {}
        result["insecure"] = from_bool(self.insecure)
        if self.mtls_key is not None:
            result["mtlsKey"] = from_union([from_str, from_none], self.mtls_key)
        return result


class Scheme(Enum):
    HTTP = "http"
    HTTPS = "https"


@dataclass
class URL:
    host: str
    path: str
    query: List[str]
    scheme: Scheme

    @staticmethod
    def from_dict(obj: Any) -> 'URL':
        assert isinstance(obj, dict)
        host = from_str(obj.get("host"))
        path = from_str(obj.get("path"))
        query = from_list(from_str, obj.get("query"))
        scheme = Scheme(obj.get("scheme"))
        return URL(host, path, query, scheme)

    def to_dict(self) -> dict:
        result: dict = {}
        result["host"] = from_str(self.host)
        result["path"] = from_str(self.path)
        result["query"] = from_list(from_str, self.query)
        result["scheme"] = to_enum(Scheme, self.scheme)
        return result


@dataclass
class RESTAPIClientConfig:
    """REST API Client service configuration"""

    body: Body
    enabled: bool
    headers: List[Header]
    jpeg: JPEG
    method: Method
    persistency: Persistency
    retries: int
    send_individual_requests: bool
    send_without_ocr: bool
    timeout: int
    tls: TLS
    url: URL

    @staticmethod
    def from_dict(obj: Any) -> 'RESTAPIClientConfig':
        assert isinstance(obj, dict)
        body = Body.from_dict(obj.get("body"))
        enabled = from_bool(obj.get("enabled"))
        headers = from_list(Header.from_dict, obj.get("headers"))
        jpeg = JPEG.from_dict(obj.get("jpeg"))
        method = Method(obj.get("method"))
        persistency = Persistency.from_dict(obj.get("persistency"))
        retries = from_int(obj.get("retries"))
        send_individual_requests = from_bool(obj.get("sendIndividualRequests"))
        send_without_ocr = from_bool(obj.get("sendWithoutOcr"))
        timeout = from_int(obj.get("timeout"))
        tls = TLS.from_dict(obj.get("tls"))
        url = URL.from_dict(obj.get("url"))
        return RESTAPIClientConfig(body, enabled, headers, jpeg, method, persistency, retries, send_individual_requests, send_without_ocr, timeout, tls, url)

    def to_dict(self) -> dict:
        result: dict = {}
        result["body"] = to_class(Body, self.body)
        result["enabled"] = from_bool(self.enabled)
        result["headers"] = from_list(lambda x: to_class(Header, x), self.headers)
        result["jpeg"] = to_class(JPEG, self.jpeg)
        result["method"] = to_enum(Method, self.method)
        result["persistency"] = to_class(Persistency, self.persistency)
        result["retries"] = from_int(self.retries)
        result["sendIndividualRequests"] = from_bool(self.send_individual_requests)
        result["sendWithoutOcr"] = from_bool(self.send_without_ocr)
        result["timeout"] = from_int(self.timeout)
        result["tls"] = to_class(TLS, self.tls)
        result["url"] = to_class(URL, self.url)
        return result


@dataclass
class RESTAPIClientStatus:
    """REST API Client service status"""

    code: int
    disk_usage: int
    file_count: int
    message: str

    @staticmethod
    def from_dict(obj: Any) -> 'RESTAPIClientStatus':
        assert isinstance(obj, dict)
        code = from_int(obj.get("code"))
        disk_usage = from_int(obj.get("diskUsage"))
        file_count = from_int(obj.get("fileCount"))
        message = from_str(obj.get("message"))
        return RESTAPIClientStatus(code, disk_usage, file_count, message)

    def to_dict(self) -> dict:
        result: dict = {}
        result["code"] = from_int(self.code)
        result["diskUsage"] = from_int(self.disk_usage)
        result["fileCount"] = from_int(self.file_count)
        result["message"] = from_str(self.message)
        return result


@dataclass
class AnalyticsClassifier:
    customer: Optional[str] = None
    max_connections: Optional[int] = None
    max_threads: Optional[int] = None
    serial: Optional[str] = None
    sha1: Optional[str] = None
    state: Optional[int] = None
    ttl: Optional[int] = None
    version: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'AnalyticsClassifier':
        assert isinstance(obj, dict)
        customer = from_union([from_str, from_none], obj.get("customer"))
        max_connections = from_union([from_int, from_none], obj.get("maxConnections"))
        max_threads = from_union([from_int, from_none], obj.get("maxThreads"))
        serial = from_union([from_str, from_none], obj.get("serial"))
        sha1 = from_union([from_str, from_none], obj.get("sha1"))
        state = from_union([from_int, from_none], obj.get("state"))
        ttl = from_union([from_int, from_none], obj.get("ttl"))
        version = from_union([from_str, from_none], obj.get("version"))
        return AnalyticsClassifier(customer, max_connections, max_threads, serial, sha1, state, ttl, version)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.customer is not None:
            result["customer"] = from_union([from_str, from_none], self.customer)
        if self.max_connections is not None:
            result["maxConnections"] = from_union([from_int, from_none], self.max_connections)
        if self.max_threads is not None:
            result["maxThreads"] = from_union([from_int, from_none], self.max_threads)
        if self.serial is not None:
            result["serial"] = from_union([from_str, from_none], self.serial)
        if self.sha1 is not None:
            result["sha1"] = from_union([from_str, from_none], self.sha1)
        if self.state is not None:
            result["state"] = from_union([from_int, from_none], self.state)
        if self.ttl is not None:
            result["ttl"] = from_union([from_int, from_none], self.ttl)
        if self.version is not None:
            result["version"] = from_union([from_str, from_none], self.version)
        return result


@dataclass
class AnalyticsOcr:
    customer: Optional[str] = None
    max_connections: Optional[int] = None
    max_threads: Optional[int] = None
    serial: Optional[str] = None
    sha1: Optional[str] = None
    state: Optional[int] = None
    ttl: Optional[int] = None
    version: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'AnalyticsOcr':
        assert isinstance(obj, dict)
        customer = from_union([from_str, from_none], obj.get("customer"))
        max_connections = from_union([from_int, from_none], obj.get("maxConnections"))
        max_threads = from_union([from_int, from_none], obj.get("maxThreads"))
        serial = from_union([from_str, from_none], obj.get("serial"))
        sha1 = from_union([from_str, from_none], obj.get("sha1"))
        state = from_union([from_int, from_none], obj.get("state"))
        ttl = from_union([from_int, from_none], obj.get("ttl"))
        version = from_union([from_str, from_none], obj.get("version"))
        return AnalyticsOcr(customer, max_connections, max_threads, serial, sha1, state, ttl, version)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.customer is not None:
            result["customer"] = from_union([from_str, from_none], self.customer)
        if self.max_connections is not None:
            result["maxConnections"] = from_union([from_int, from_none], self.max_connections)
        if self.max_threads is not None:
            result["maxThreads"] = from_union([from_int, from_none], self.max_threads)
        if self.serial is not None:
            result["serial"] = from_union([from_str, from_none], self.serial)
        if self.sha1 is not None:
            result["sha1"] = from_union([from_str, from_none], self.sha1)
        if self.state is not None:
            result["state"] = from_union([from_int, from_none], self.state)
        if self.ttl is not None:
            result["ttl"] = from_union([from_int, from_none], self.ttl)
        if self.version is not None:
            result["version"] = from_union([from_str, from_none], self.version)
        return result


@dataclass
class Analytics:
    classifier: Optional[AnalyticsClassifier] = None
    ocr: Optional[AnalyticsOcr] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Analytics':
        assert isinstance(obj, dict)
        classifier = from_union([AnalyticsClassifier.from_dict, from_none], obj.get("classifier"))
        ocr = from_union([AnalyticsOcr.from_dict, from_none], obj.get("ocr"))
        return Analytics(classifier, ocr)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.classifier is not None:
            result["classifier"] = from_union([lambda x: to_class(AnalyticsClassifier, x), from_none], self.classifier)
        if self.ocr is not None:
            result["ocr"] = from_union([lambda x: to_class(AnalyticsOcr, x), from_none], self.ocr)
        return result


@dataclass
class DeviceID:
    serial: Optional[str] = None

    @staticmethod
    def from_dict(obj: Any) -> 'DeviceID':
        assert isinstance(obj, dict)
        serial = from_union([from_str, from_none], obj.get("serial"))
        return DeviceID(serial)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.serial is not None:
            result["serial"] = from_union([from_str, from_none], self.serial)
        return result


@dataclass
class Licenses:
    """License service information"""

    analytics: Optional[Analytics] = None
    device_id: Optional[DeviceID] = None

    @staticmethod
    def from_dict(obj: Any) -> 'Licenses':
        assert isinstance(obj, dict)
        analytics = from_union([Analytics.from_dict, from_none], obj.get("analytics"))
        device_id = from_union([DeviceID.from_dict, from_none], obj.get("deviceId"))
        return Licenses(analytics, device_id)

    def to_dict(self) -> dict:
        result: dict = {}
        if self.analytics is not None:
            result["analytics"] = from_union([lambda x: to_class(Analytics, x), from_none], self.analytics)
        if self.device_id is not None:
            result["deviceId"] = from_union([lambda x: to_class(DeviceID, x), from_none], self.device_id)
        return result


def profile_config_from_dict(s: Any) -> ProfileConfig:
    return ProfileConfig.from_dict(s)


def profile_config_to_dict(x: ProfileConfig) -> Any:
    return to_class(ProfileConfig, x)


def ocr_config_from_dict(s: Any) -> OcrConfig:
    return OcrConfig.from_dict(s)


def ocr_config_to_dict(x: OcrConfig) -> Any:
    return to_class(OcrConfig, x)


def analytics_config_from_dict(s: Any) -> AnalyticsConfig:
    return AnalyticsConfig.from_dict(s)


def analytics_config_to_dict(x: AnalyticsConfig) -> Any:
    return to_class(AnalyticsConfig, x)


def classifier_config_from_dict(s: Any) -> ClassifierConfig:
    return ClassifierConfig.from_dict(s)


def classifier_config_to_dict(x: ClassifierConfig) -> Any:
    return to_class(ClassifierConfig, x)


def auto_focus_from_dict(s: Any) -> AutoFocus:
    return AutoFocus.from_dict(s)


def auto_focus_to_dict(x: AutoFocus) -> Any:
    return to_class(AutoFocus, x)


def stream_config_from_dict(s: Any) -> StreamConfig:
    return StreamConfig.from_dict(s)


def stream_config_to_dict(x: StreamConfig) -> Any:
    return to_class(StreamConfig, x)


def misc_from_dict(s: Any) -> Misc:
    return Misc.from_dict(s)


def misc_to_dict(x: Misc) -> Any:
    return to_class(Misc, x)


def misc_volatile_from_dict(s: Any) -> MiscVolatile:
    return MiscVolatile.from_dict(s)


def misc_volatile_to_dict(x: MiscVolatile) -> Any:
    return to_class(MiscVolatile, x)


def itscampro_config_from_dict(s: Any) -> ItscamproConfig:
    return ItscamproConfig.from_dict(s)


def itscampro_config_to_dict(x: ItscamproConfig) -> Any:
    return to_class(ItscamproConfig, x)


def itscampro_status_from_dict(s: Any) -> ItscamproStatus:
    return ItscamproStatus.from_dict(s)


def itscampro_status_to_dict(x: ItscamproStatus) -> Any:
    return to_class(ItscamproStatus, x)


def image_sign_config_from_dict(s: Any) -> ImageSignConfig:
    return ImageSignConfig.from_dict(s)


def image_sign_config_to_dict(x: ImageSignConfig) -> Any:
    return to_class(ImageSignConfig, x)


def ftp_config_from_dict(s: Any) -> FTPConfig:
    return FTPConfig.from_dict(s)


def ftp_config_to_dict(x: FTPConfig) -> Any:
    return to_class(FTPConfig, x)


def lince_config_from_dict(s: Any) -> LinceConfig:
    return LinceConfig.from_dict(s)


def lince_config_to_dict(x: LinceConfig) -> Any:
    return to_class(LinceConfig, x)


def lince_status_from_dict(s: Any) -> LinceStatus:
    return LinceStatus.from_dict(s)


def lince_status_to_dict(x: LinceStatus) -> Any:
    return to_class(LinceStatus, x)


def vehicle_indicator_config_from_dict(s: Any) -> VehicleIndicatorConfig:
    return VehicleIndicatorConfig.from_dict(s)


def vehicle_indicator_config_to_dict(x: VehicleIndicatorConfig) -> Any:
    return to_class(VehicleIndicatorConfig, x)


def protocols_config_from_dict(s: Any) -> ProtocolsConfig:
    return ProtocolsConfig.from_dict(s)


def protocols_config_to_dict(x: ProtocolsConfig) -> Any:
    return to_class(ProtocolsConfig, x)


def profile_transitioner_from_dict(s: Any) -> ProfileTransitioner:
    return ProfileTransitioner.from_dict(s)


def profile_transitioner_to_dict(x: ProfileTransitioner) -> Any:
    return to_class(ProfileTransitioner, x)


def lanes_config_from_dict(s: Any) -> LanesConfig:
    return LanesConfig.from_dict(s)


def lanes_config_to_dict(x: LanesConfig) -> Any:
    return to_class(LanesConfig, x)


def io_config_from_dict(s: Any) -> IoConfig:
    return IoConfig.from_dict(s)


def io_config_to_dict(x: IoConfig) -> Any:
    return to_class(IoConfig, x)


def io_basic_from_dict(s: Any) -> IoBasic:
    return IoBasic.from_dict(s)


def io_basic_to_dict(x: IoBasic) -> Any:
    return to_class(IoBasic, x)


def restapi_client_config_from_dict(s: Any) -> RESTAPIClientConfig:
    return RESTAPIClientConfig.from_dict(s)


def restapi_client_config_to_dict(x: RESTAPIClientConfig) -> Any:
    return to_class(RESTAPIClientConfig, x)


def restapi_client_status_from_dict(s: Any) -> RESTAPIClientStatus:
    return RESTAPIClientStatus.from_dict(s)


def restapi_client_status_to_dict(x: RESTAPIClientStatus) -> Any:
    return to_class(RESTAPIClientStatus, x)


def licenses_from_dict(s: Any) -> Licenses:
    return Licenses.from_dict(s)


def licenses_to_dict(x: Licenses) -> Any:
    return to_class(Licenses, x)

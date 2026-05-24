"""
ITSCAM SDK - REST API client (Python)

Thin Pythonic wrapper around the C REST client.

The client exposes two coexisting surfaces:

* **Typed helpers** (preferred) -- ``get_ocr_config()``, ``set_ocr_config(cfg)``,
  ``get_profiles()`` etc.  These return dataclasses generated from the
  camera's OpenAPI document (see ``itscam.rest_types`` and
  ``tools/codegen/`` in the SDK repo).
* **Generic verbs** (escape hatch) -- ``get(path)``, ``put(path, body)``,
  ``post(path, body)``, ``delete(path)``.  These return parsed JSON
  (``dict`` / ``list``) or the raw response string when the body is not
  valid JSON.  Use them for endpoints that are not (yet) typed.

* **Partial PUT** -- the ITSCAM daemon merges PUT bodies into the existing
  configuration.  Typed setters (e.g. ``update_profile_by_id()``,
  ``set_ocr_config()``) use partial serialization: only fields that are not
  ``None`` are included in the PUT body.  Construct a dataclass with only
  the fields you want to change and pass it directly.  The generic
  ``patch_json()`` / ``put()`` methods remain available for untyped payloads.

Example usage::

    from itscam import ItscamRestClient

    with ItscamRestClient() as rest:
        rest.set_base_url("camera.example.com", port=443, scheme="https")
        rest.set_verify_server_certificate(False)
        rest.login("admin", "password")

        # Typed surface:
        info = rest.get_volatile_info()
        print(info.cpu_temperature)

        # Or, for endpoints not yet typed:
        info = rest.get("/api/equipment/misc/readonly/constants")

Copyright (c) 2026 Pumatronix
"""
from __future__ import annotations

import json
from typing import Any, List, Optional, Union

from . import bindings, rest_types as _rt
from .types import _raise_for_error


JsonValue = Union[dict, list, str, int, float, bool, None]


def _take_string(handle) -> str:
    """Read an ITSCAM_String*, free it, return the contained UTF-8 string."""
    if not handle:
        return ""
    lib = bindings.get_lib()
    try:
        ptr = lib.ITSCAM_String_data(handle)
        return ptr.decode("utf-8") if ptr else ""
    finally:
        lib.ITSCAM_String_destroy(handle)


def _decode(body: str) -> JsonValue:
    if not body:
        return None
    try:
        return json.loads(body)
    except json.JSONDecodeError:
        return body


def _c_str(s: Optional[str]) -> Optional[bytes]:
    return s.encode("utf-8") if s is not None else None


class ItscamRestClient:
    """Pythonic wrapper for the ITSCAM REST API."""

    def __init__(self) -> None:
        self._lib = bindings.get_lib()
        self._handle = self._lib.ITSCAM_RestClient_create()
        if not self._handle:
            raise MemoryError("ITSCAM_RestClient_create returned NULL")

    # ----- context manager ---------------------------------------------------

    def __enter__(self) -> "ItscamRestClient":
        return self

    def __exit__(self, *exc_info) -> None:
        self.close()

    def close(self) -> None:
        if getattr(self, "_handle", None):
            self._lib.ITSCAM_RestClient_destroy(self._handle)
            self._handle = None

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:
            pass

    # ----- configuration -----------------------------------------------------

    def set_base_url(self, host: str, port: int = 80,
                     scheme: str = "http") -> None:
        rc = self._lib.ITSCAM_RestClient_setBaseUrl(
            self._handle, _c_str(host), port, _c_str(scheme))
        _raise_for_error(rc, f"set_base_url({host}:{port})")

    def set_api_prefix(self, prefix: str) -> None:
        self._lib.ITSCAM_RestClient_setApiPrefix(
            self._handle, _c_str(prefix))

    def set_ca_cert_file(self, pem_path: str) -> None:
        self._lib.ITSCAM_RestClient_setCaCertFile(
            self._handle, _c_str(pem_path))

    def set_ca_cert_data(self, pem: str) -> None:
        self._lib.ITSCAM_RestClient_setCaCertData(
            self._handle, _c_str(pem))

    def set_verify_server_certificate(self, verify: bool) -> None:
        self._lib.ITSCAM_RestClient_setVerifyServerCertificate(
            self._handle, 1 if verify else 0)

    def set_client_certificate(self, cert_pem: str, key_pem: str) -> None:
        self._lib.ITSCAM_RestClient_setClientCertificate(
            self._handle, _c_str(cert_pem), _c_str(key_pem))

    # ----- authentication ----------------------------------------------------

    def login(self, username: str, password: str,
              timeout_ms: int = 10000) -> JsonValue:
        out = bindings.ITSCAM_String()
        rc = self._lib.ITSCAM_RestClient_login(
            self._handle, _c_str(username), _c_str(password),
            timeout_ms, bindings.byref(out))
        body = _take_string(out)
        _raise_for_error(rc, "login")
        return _decode(body)

    def set_auth_token(self, token: str) -> None:
        self._lib.ITSCAM_RestClient_setAuthToken(self._handle, _c_str(token))

    def clear_auth_token(self) -> None:
        self._lib.ITSCAM_RestClient_clearAuthToken(self._handle)

    # ----- HTTP verbs --------------------------------------------------------

    def get(self, path: str, timeout_ms: int = 10000) -> JsonValue:
        out = bindings.ITSCAM_String()
        rc = self._lib.ITSCAM_RestClient_httpGet(
            self._handle, _c_str(path), timeout_ms, bindings.byref(out))
        body = _take_string(out)
        _raise_for_error(rc, f"GET {path}")
        return _decode(body)

    def put(self, path: str, body: Any,
            timeout_ms: int = 10000) -> JsonValue:
        return self._with_body("Put", path, body, timeout_ms)

    def patch_json(self, path: str, patch: Any,
                   timeout_ms: int = 10000) -> JsonValue:
        """PUT a partial JSON document; the daemon merges ``patch`` in place.

        Typed setters already use partial serialization (``None`` fields are
        omitted), so ``patch_json()`` is mainly useful for endpoints without
        a typed helper or for hand-built patches.
        """
        return self.put(path, patch, timeout_ms)

    def post(self, path: str, body: Any,
             timeout_ms: int = 10000) -> JsonValue:
        return self._with_body("Post", path, body, timeout_ms)

    def delete(self, path: str, timeout_ms: int = 10000) -> JsonValue:
        out = bindings.ITSCAM_String()
        rc = self._lib.ITSCAM_RestClient_httpDelete(
            self._handle, _c_str(path), timeout_ms, bindings.byref(out))
        body = _take_string(out)
        _raise_for_error(rc, f"DELETE {path}")
        return _decode(body)

    def _with_body(self, verb: str, path: str, body: Any,
                   timeout_ms: int) -> JsonValue:
        fn = getattr(self._lib, "ITSCAM_RestClient_http" + verb)
        if isinstance(body, (dict, list)):
            body_str = json.dumps(body)
        elif body is None:
            body_str = ""
        else:
            body_str = str(body)
        out = bindings.ITSCAM_String()
        rc = fn(self._handle, _c_str(path), _c_str(body_str),
                timeout_ms, bindings.byref(out))
        resp = _take_string(out)
        _raise_for_error(rc, f"{verb.upper()} {path}")
        return _decode(resp)

    # ----- Typed helpers (auto-generated POCOs) ------------------------------
    #
    # These wrap the generic ``get`` / ``put`` verbs above and round-trip JSON
    # through the dataclasses in ``itscam.rest_types``.  The dataclasses are
    # regenerated from the camera's OpenAPI document by ``make codegen`` --
    # see ``tools/codegen/`` for the maintainer + end-user workflows.
    #
    # Unknown JSON fields are silently dropped by ``from_dict`` (it asserts
    # on type but ignores extras), so newer-firmware fields survive
    # GET / inspect but are lost on a full round-trip SET.  Use the generic
    # verbs for partial updates that need to preserve unknown fields.

    @staticmethod
    def _list_from_dict(cls, raw):
        if not isinstance(raw, list):
            raise TypeError(f"Expected JSON array, got {type(raw).__name__}")
        return [cls.from_dict(x) for x in raw]

    # --- Image profiles ------------------------------------------------------

    def get_profiles(self, timeout_ms: int = 10000) -> List[_rt.ProfileConfig]:
        return self._list_from_dict(
            _rt.ProfileConfig, self.get("/api/image/profiles", timeout_ms))

    def get_profile(self, profile_id: int,
                    timeout_ms: int = 10000) -> List[_rt.ProfileConfig]:
        raw = self.get(f"/api/image/profiles?id={profile_id}", timeout_ms)
        return self._list_from_dict(_rt.ProfileConfig, raw)

    def create_profile(self, profile: _rt.ProfileConfig,
                       timeout_ms: int = 10000) -> _rt.ProfileConfig:
        raw = self.post("/api/image/profiles", profile.to_dict(), timeout_ms)
        return _rt.ProfileConfig.from_dict(raw)

    # Typed setters use partial serialization: ``to_dict()`` omits ``None``
    # fields, so only explicitly-set values are included in PUT bodies.

    def update_profile_by_id(self, profile_id: int,
                             profile: _rt.ProfileConfig,
                             timeout_ms: int = 10000) -> _rt.ProfileConfig:
        """PUT a partial ``ProfileConfig`` document.

        Only non-``None`` fields are included in the PUT body.  Construct a
        ``ProfileConfig`` with only the fields you want to change.
        """
        raw = self.put(f"/api/image/profiles/{profile_id}",
                       profile.to_dict(), timeout_ms)
        return _rt.ProfileConfig.from_dict(raw)

    def update_profiles(self, profiles: List[_rt.ProfileConfig],
                        timeout_ms: int = 10000) -> _rt.ProfileConfig:
        """PUT a JSON array of profiles (bulk endpoint).

        Each profile is partially serialized (``None`` fields omitted).
        """
        body = [p.to_dict() for p in profiles]
        raw = self.put("/api/image/profiles", body, timeout_ms)
        return _rt.ProfileConfig.from_dict(raw)

    def delete_profile(self, profile_id: int,
                       timeout_ms: int = 10000) -> JsonValue:
        return self.delete(f"/api/image/profiles?id={profile_id}", timeout_ms)

    # --- Equipment misc ------------------------------------------------------

    def get_volatile_info(self,
                          timeout_ms: int = 10000) -> _rt.MiscVolatile:
        raw = self.get("/api/equipment/misc/readonly/volatile", timeout_ms)
        return _rt.MiscVolatile.from_dict(raw)

    def get_misc(self, timeout_ms: int = 10000) -> _rt.Misc:
        return _rt.Misc.from_dict(self.get("/api/equipment/misc", timeout_ms))

    def set_misc(self, config: _rt.Misc,
                 timeout_ms: int = 10000) -> _rt.Misc:
        raw = self.put("/api/equipment/misc", config.to_dict(), timeout_ms)
        return _rt.Misc.from_dict(raw)

    # --- OCR -----------------------------------------------------------------

    def get_ocr_config(self, timeout_ms: int = 10000) -> _rt.OcrConfig:
        return _rt.OcrConfig.from_dict(
            self.get("/api/equipment/ocr", timeout_ms))

    def set_ocr_config(self, config: _rt.OcrConfig,
                       timeout_ms: int = 10000) -> _rt.OcrConfig:
        raw = self.put("/api/equipment/ocr", config.to_dict(), timeout_ms)
        return _rt.OcrConfig.from_dict(raw)

    # --- Analytics -----------------------------------------------------------

    def get_analytics_config(self,
                             timeout_ms: int = 10000) -> _rt.AnalyticsConfig:
        return _rt.AnalyticsConfig.from_dict(
            self.get("/api/equipment/analytics", timeout_ms))

    def set_analytics_config(self, config: _rt.AnalyticsConfig,
                             timeout_ms: int = 10000) -> _rt.AnalyticsConfig:
        raw = self.put("/api/equipment/analytics", config.to_dict(),
                       timeout_ms)
        return _rt.AnalyticsConfig.from_dict(raw)

    # --- Classifier ----------------------------------------------------------

    def get_classifier_config(
        self, timeout_ms: int = 10000
    ) -> _rt.ClassifierConfig:
        return _rt.ClassifierConfig.from_dict(
            self.get("/api/equipment/classifier", timeout_ms))

    def set_classifier_config(
        self, config: _rt.ClassifierConfig, timeout_ms: int = 10000
    ) -> _rt.ClassifierConfig:
        raw = self.put("/api/equipment/classifier", config.to_dict(),
                       timeout_ms)
        return _rt.ClassifierConfig.from_dict(raw)

    # --- AutoFocus -----------------------------------------------------------

    def get_autofocus(self, timeout_ms: int = 10000) -> _rt.AutoFocus:
        return _rt.AutoFocus.from_dict(
            self.get("/api/equipment/autofocus", timeout_ms))

    def set_autofocus(self, config: _rt.AutoFocus,
                      timeout_ms: int = 10000) -> _rt.AutoFocus:
        raw = self.put("/api/equipment/autofocus", config.to_dict(),
                       timeout_ms)
        return _rt.AutoFocus.from_dict(raw)

    # --- Video streams -------------------------------------------------------

    def get_stream_config(self,
                          timeout_ms: int = 10000) -> _rt.StreamConfig:
        return _rt.StreamConfig.from_dict(
            self.get("/api/video/streams", timeout_ms))

    def set_stream_config(self, config: _rt.StreamConfig,
                          timeout_ms: int = 10000) -> _rt.StreamConfig:
        raw = self.put("/api/video/streams", config.to_dict(), timeout_ms)
        return _rt.StreamConfig.from_dict(raw)

    # --- Lanes ---------------------------------------------------------------

    def get_lanes_config(self,
                         timeout_ms: int = 10000) -> _rt.LanesConfig:
        return _rt.LanesConfig.from_dict(
            self.get("/api/equipment/lanes", timeout_ms))

    def set_lanes_config(self, config: _rt.LanesConfig,
                         timeout_ms: int = 10000) -> _rt.LanesConfig:
        raw = self.put("/api/equipment/lanes", config.to_dict(), timeout_ms)
        return _rt.LanesConfig.from_dict(raw)

    # --- ITSCAM PRO server ---------------------------------------------------

    def get_itscampro_config(
        self, timeout_ms: int = 10000
    ) -> _rt.ItscamproConfig:
        return _rt.ItscamproConfig.from_dict(
            self.get("/api/equipment/servers/itscampro", timeout_ms))

    def set_itscampro_config(
        self, config: _rt.ItscamproConfig, timeout_ms: int = 10000
    ) -> _rt.ItscamproConfig:
        raw = self.put("/api/equipment/servers/itscampro", config.to_dict(),
                       timeout_ms)
        return _rt.ItscamproConfig.from_dict(raw)

    def get_itscampro_status(
        self, timeout_ms: int = 10000
    ) -> _rt.ItscamproStatus:
        return _rt.ItscamproStatus.from_dict(
            self.get("/api/equipment/servers/itscampro/status", timeout_ms))

    # --- Image sign ----------------------------------------------------------

    def get_image_sign_config(
        self, timeout_ms: int = 10000
    ) -> _rt.ImageSignConfig:
        return _rt.ImageSignConfig.from_dict(
            self.get("/api/equipment/imageSign", timeout_ms))

    # --- FTP -----------------------------------------------------------------

    def get_ftp_config(self, timeout_ms: int = 10000) -> _rt.FTPConfig:
        return _rt.FTPConfig.from_dict(
            self.get("/api/equipment/servers/ftp", timeout_ms))

    def set_ftp_config(self, config: _rt.FTPConfig,
                       timeout_ms: int = 10000) -> _rt.FTPConfig:
        raw = self.put("/api/equipment/servers/ftp", config.to_dict(),
                       timeout_ms)
        return _rt.FTPConfig.from_dict(raw)

    # --- Lince ---------------------------------------------------------------

    def get_lince_config(self,
                         timeout_ms: int = 10000) -> _rt.LinceConfig:
        return _rt.LinceConfig.from_dict(
            self.get("/api/equipment/servers/lince", timeout_ms))

    def set_lince_config(self, config: _rt.LinceConfig,
                         timeout_ms: int = 10000) -> _rt.LinceConfig:
        raw = self.put("/api/equipment/servers/lince", config.to_dict(),
                       timeout_ms)
        return _rt.LinceConfig.from_dict(raw)

    def get_lince_status(self,
                         timeout_ms: int = 10000) -> _rt.LinceStatus:
        return _rt.LinceStatus.from_dict(
            self.get("/api/equipment/servers/lince/status", timeout_ms))

    # --- Vehicle indicator ---------------------------------------------------

    def get_vehicle_indicator_config(
        self, timeout_ms: int = 10000
    ) -> _rt.VehicleIndicatorConfig:
        return _rt.VehicleIndicatorConfig.from_dict(
            self.get("/api/equipment/vehicleIndicator", timeout_ms))

    def set_vehicle_indicator_config(
        self, config: _rt.VehicleIndicatorConfig, timeout_ms: int = 10000
    ) -> _rt.VehicleIndicatorConfig:
        raw = self.put("/api/equipment/vehicleIndicator", config.to_dict(),
                       timeout_ms)
        return _rt.VehicleIndicatorConfig.from_dict(raw)

    # --- Output protocols ----------------------------------------------------

    def get_protocols_config(
        self, timeout_ms: int = 10000
    ) -> _rt.ProtocolsConfig:
        return _rt.ProtocolsConfig.from_dict(
            self.get("/api/equipment/servers/protocols", timeout_ms))

    def set_protocols_config(
        self, config: _rt.ProtocolsConfig, timeout_ms: int = 10000
    ) -> _rt.ProtocolsConfig:
        raw = self.put("/api/equipment/servers/protocols", config.to_dict(),
                       timeout_ms)
        return _rt.ProtocolsConfig.from_dict(raw)

    # --- Profile transitioner ------------------------------------------------

    def get_profile_transitioner(
        self, timeout_ms: int = 10000
    ) -> _rt.ProfileTransitioner:
        return _rt.ProfileTransitioner.from_dict(
            self.get("/api/equipment/transitioner", timeout_ms))

    def set_profile_transitioner(
        self, config: _rt.ProfileTransitioner, timeout_ms: int = 10000
    ) -> _rt.ProfileTransitioner:
        raw = self.put("/api/equipment/transitioner", config.to_dict(),
                       timeout_ms)
        return _rt.ProfileTransitioner.from_dict(raw)

    # --- I/O ports -----------------------------------------------------------

    def get_io_ports(self, timeout_ms: int = 10000) -> List[_rt.IoConfig]:
        return self._list_from_dict(
            _rt.IoConfig, self.get("/api/equipment/ioPorts", timeout_ms))

    def set_io_ports(self, ports: List[_rt.IoConfig],
                     timeout_ms: int = 10000) -> List[_rt.IoConfig]:
        body = [p.to_dict() for p in ports]
        raw = self.put("/api/equipment/ioPorts", body, timeout_ms)
        return self._list_from_dict(_rt.IoConfig, raw)

    def get_io_port(self, port_id: int,
                    timeout_ms: int = 10000) -> _rt.IoConfig:
        return _rt.IoConfig.from_dict(
            self.get(f"/api/equipment/ioPorts/{port_id}", timeout_ms))

    def set_io_port(self, port_id: int, port: _rt.IoConfig,
                    timeout_ms: int = 10000) -> _rt.IoConfig:
        raw = self.put(f"/api/equipment/ioPorts/{port_id}",
                       port.to_dict(), timeout_ms)
        return _rt.IoConfig.from_dict(raw)

    def get_io_basic(self, timeout_ms: int = 10000) -> List[_rt.IoBasic]:
        return self._list_from_dict(
            _rt.IoBasic, self.get("/api/equipment/ioBasic", timeout_ms))

    def set_io_basic(self, ports: List[_rt.IoBasic],
                     timeout_ms: int = 10000) -> List[_rt.IoBasic]:
        body = [p.to_dict() for p in ports]
        raw = self.put("/api/equipment/ioBasic", body, timeout_ms)
        return self._list_from_dict(_rt.IoBasic, raw)

    # --- REST API client (webhook) servers ----------------------------------

    def get_restapi_client_config(
        self, server_id: int, timeout_ms: int = 10000
    ) -> _rt.RESTAPIClientConfig:
        return _rt.RESTAPIClientConfig.from_dict(self.get(
            f"/api/equipment/servers/restapiclient/{server_id}/config",
            timeout_ms))

    def set_restapi_client_config(
        self, server_id: int, config: _rt.RESTAPIClientConfig,
        timeout_ms: int = 10000
    ) -> _rt.RESTAPIClientConfig:
        raw = self.put(
            f"/api/equipment/servers/restapiclient/{server_id}/config",
            config.to_dict(), timeout_ms)
        return _rt.RESTAPIClientConfig.from_dict(raw)

    def get_restapi_client_status(
        self, server_id: int, timeout_ms: int = 10000
    ) -> _rt.RESTAPIClientStatus:
        return _rt.RESTAPIClientStatus.from_dict(self.get(
            f"/api/equipment/servers/restapiclient/{server_id}/status",
            timeout_ms))

    # --- Licenses ------------------------------------------------------------

    def get_licenses(self, timeout_ms: int = 10000) -> _rt.Licenses:
        return _rt.Licenses.from_dict(
            self.get("/api/system/licenses", timeout_ms))

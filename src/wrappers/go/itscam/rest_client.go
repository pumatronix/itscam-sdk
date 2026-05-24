// Package itscam - REST API client (Go wrapper).
//
// RestClient wraps the ITSCAM webapp JSON REST API.  Two coexisting
// surfaces:
//
//   * Typed helpers -- e.g. GetOcrConfig / SetOcrConfig -- return Go
//     structs generated from the camera's OpenAPI document.
//   * Generic verbs -- Get / Put / Post / Delete return raw JSON.
//
// Partial PUT: the ITSCAM daemon merges PUT bodies into the existing
// configuration.  Send only changed fields via PatchJSON / Put.  Typed
// setters and UpdateProfileById serialise full objects and fail with
// HTTP 500 on several endpoints (notably PUT /api/image/profiles/{id}).
//
// HTTPS is supported through the statically-linked mbedTLS backend; set
// the scheme to "https" in SetBaseUrl and configure CA certificates as
// needed.

package itscam

/*
#cgo !static LDFLAGS: -litscam_sdk
#cgo static,linux LDFLAGS: ${SRCDIR}/../../../core/build/linux/libitscam_sdk.a -lstdc++ -lpthread -lm
#cgo static,windows CFLAGS: -DITSCAM_SDK_STATIC
#cgo static,windows LDFLAGS: ${SRCDIR}/../../../core/build/win-x64/libitscam_sdk_static.a -lws2_32 -lstdc++ -lm -static
#include <stdlib.h>
#include "../../../core/c_api/itscam_rest_client_c.h"
*/
import "C"
import (
	"encoding/json"
	"errors"
	"fmt"
	"runtime"
	"sync"
	"unsafe"
)

// RestClient is the Go wrapper for ItscamRestClient.
type RestClient struct {
	handle *C.ITSCAM_RestClient
	mu     sync.Mutex
	closed bool
}

// NewRestClient creates a new REST client.
func NewRestClient() (*RestClient, error) {
	h := C.ITSCAM_RestClient_create()
	if h == nil {
		return nil, errors.New("failed to allocate ITSCAM REST client")
	}
	r := &RestClient{handle: h}
	runtime.SetFinalizer(r, (*RestClient).Close)
	return r, nil
}

// Close releases the client.  Safe to call multiple times.
func (r *RestClient) Close() error {
	r.mu.Lock()
	defer r.mu.Unlock()
	if r.closed {
		return nil
	}
	r.closed = true
	if r.handle != nil {
		C.ITSCAM_RestClient_destroy(r.handle)
		r.handle = nil
	}
	runtime.SetFinalizer(r, nil)
	return nil
}

// ============================================================================
//  Configuration
// ============================================================================

// SetBaseUrl configures the camera host, port and scheme.  Pass "https"
// to use TLS; pass port = 0 to select the protocol default (80 / 443).
func (r *RestClient) SetBaseUrl(host string, port uint16, scheme string) error {
	r.mu.Lock()
	defer r.mu.Unlock()
	cHost := C.CString(host)
	defer C.free(unsafe.Pointer(cHost))
	cScheme := C.CString(scheme)
	defer C.free(unsafe.Pointer(cScheme))
	return errorFromCode(int(C.ITSCAM_RestClient_setBaseUrl(
		r.handle, cHost, C.uint16_t(port), cScheme)))
}

func (r *RestClient) SetApiPrefix(prefix string) {
	r.mu.Lock()
	defer r.mu.Unlock()
	c := C.CString(prefix)
	defer C.free(unsafe.Pointer(c))
	C.ITSCAM_RestClient_setApiPrefix(r.handle, c)
}

func (r *RestClient) SetCaCertFile(path string) {
	r.mu.Lock()
	defer r.mu.Unlock()
	c := C.CString(path)
	defer C.free(unsafe.Pointer(c))
	C.ITSCAM_RestClient_setCaCertFile(r.handle, c)
}

func (r *RestClient) SetCaCertData(pem string) {
	r.mu.Lock()
	defer r.mu.Unlock()
	c := C.CString(pem)
	defer C.free(unsafe.Pointer(c))
	C.ITSCAM_RestClient_setCaCertData(r.handle, c)
}

func (r *RestClient) SetVerifyServerCertificate(verify bool) {
	r.mu.Lock()
	defer r.mu.Unlock()
	v := C.int(0)
	if verify {
		v = 1
	}
	C.ITSCAM_RestClient_setVerifyServerCertificate(r.handle, v)
}

func (r *RestClient) SetClientCertificate(certPem, keyPem string) {
	r.mu.Lock()
	defer r.mu.Unlock()
	cCert := C.CString(certPem)
	defer C.free(unsafe.Pointer(cCert))
	cKey := C.CString(keyPem)
	defer C.free(unsafe.Pointer(cKey))
	C.ITSCAM_RestClient_setClientCertificate(r.handle, cCert, cKey)
}

// ============================================================================
//  Authentication
// ============================================================================

// Login posts credentials to /api/auth and stores the returned bearer
// token internally.  Returns the response body as a JSON string.
func (r *RestClient) Login(username, password string, timeoutMs uint32) (string, error) {
	r.mu.Lock()
	defer r.mu.Unlock()
	cUser := C.CString(username)
	defer C.free(unsafe.Pointer(cUser))
	cPass := C.CString(password)
	defer C.free(unsafe.Pointer(cPass))
	var out *C.ITSCAM_String
	rc := C.ITSCAM_RestClient_login(r.handle, cUser, cPass,
		C.uint32_t(timeoutMs), &out)
	body := takeString(out)
	if err := errorFromCode(int(rc)); err != nil {
		return body, err
	}
	return body, nil
}

func (r *RestClient) SetAuthToken(token string) {
	r.mu.Lock()
	defer r.mu.Unlock()
	c := C.CString(token)
	defer C.free(unsafe.Pointer(c))
	C.ITSCAM_RestClient_setAuthToken(r.handle, c)
}

func (r *RestClient) ClearAuthToken() {
	r.mu.Lock()
	defer r.mu.Unlock()
	C.ITSCAM_RestClient_clearAuthToken(r.handle)
}

// ============================================================================
//  HTTP verbs
// ============================================================================

// Get issues a GET request and returns the JSON-decoded response.
func (r *RestClient) Get(path string, timeoutMs uint32) (string, error) {
	r.mu.Lock()
	defer r.mu.Unlock()
	cPath := C.CString(path)
	defer C.free(unsafe.Pointer(cPath))
	var out *C.ITSCAM_String
	rc := C.ITSCAM_RestClient_httpGet(r.handle, cPath,
		C.uint32_t(timeoutMs), &out)
	body := takeString(out)
	if err := errorFromCode(int(rc)); err != nil {
		return body, err
	}
	return body, nil
}

// GetJSON convenience: GET + json.Unmarshal into the supplied value.
func (r *RestClient) GetJSON(path string, timeoutMs uint32, dst interface{}) error {
	body, err := r.Get(path, timeoutMs)
	if err != nil {
		return err
	}
	if body == "" {
		return nil
	}
	return json.Unmarshal([]byte(body), dst)
}

// Put issues a PUT request with the given JSON body.  The ITSCAM daemon
// treats most endpoints as partial updates and merges the body in place.
func (r *RestClient) Put(path, jsonBody string, timeoutMs uint32) (string, error) {
	r.mu.Lock()
	defer r.mu.Unlock()
	cPath := C.CString(path)
	defer C.free(unsafe.Pointer(cPath))
	cBody := C.CString(jsonBody)
	defer C.free(unsafe.Pointer(cBody))
	var out *C.ITSCAM_String
	rc := C.ITSCAM_RestClient_httpPut(r.handle, cPath, cBody,
		C.uint32_t(timeoutMs), &out)
	body := takeString(out)
	if err := errorFromCode(int(rc)); err != nil {
		return body, err
	}
	return body, nil
}

// PatchJSON PUTs a partial JSON document.  The daemon merges the supplied
// fields.  Prefer this over UpdateProfileById when changing a subset of
// fields on an existing configuration.
func (r *RestClient) PatchJSON(path string, patch interface{}, timeoutMs uint32) (string, error) {
	return r.putJSON(path, patch, timeoutMs)
}

// Post issues a POST request with the given JSON body.
func (r *RestClient) Post(path, jsonBody string, timeoutMs uint32) (string, error) {
	r.mu.Lock()
	defer r.mu.Unlock()
	cPath := C.CString(path)
	defer C.free(unsafe.Pointer(cPath))
	cBody := C.CString(jsonBody)
	defer C.free(unsafe.Pointer(cBody))
	var out *C.ITSCAM_String
	rc := C.ITSCAM_RestClient_httpPost(r.handle, cPath, cBody,
		C.uint32_t(timeoutMs), &out)
	body := takeString(out)
	if err := errorFromCode(int(rc)); err != nil {
		return body, err
	}
	return body, nil
}

// Delete issues a DELETE request.
func (r *RestClient) Delete(path string, timeoutMs uint32) (string, error) {
	r.mu.Lock()
	defer r.mu.Unlock()
	cPath := C.CString(path)
	defer C.free(unsafe.Pointer(cPath))
	var out *C.ITSCAM_String
	rc := C.ITSCAM_RestClient_httpDelete(r.handle, cPath,
		C.uint32_t(timeoutMs), &out)
	body := takeString(out)
	if err := errorFromCode(int(rc)); err != nil {
		return body, err
	}
	return body, nil
}

// ============================================================================
//  Typed helpers
//
//  Each helper round-trips JSON through the generated rest_types.go
//  structs.  Unknown JSON fields (newer firmware, custom extensions)
//  survive a GET because encoding/json drops them on decode; they are
//  lost on the round-trip back through Set*.  Use the generic verbs +
//  json.RawMessage if you need to preserve unknown fields.
// ============================================================================

// putJSON marshals v, sends a PUT, returns the response body.
func (r *RestClient) putJSON(path string, v interface{}, timeoutMs uint32) (string, error) {
	body, err := json.Marshal(v)
	if err != nil {
		return "", err
	}
	return r.Put(path, string(body), timeoutMs)
}

// postJSON marshals v, sends a POST, returns the response body.
func (r *RestClient) postJSON(path string, v interface{}, timeoutMs uint32) (string, error) {
	body, err := json.Marshal(v)
	if err != nil {
		return "", err
	}
	return r.Post(path, string(body), timeoutMs)
}

// decodeInto unmarshals body into dst.  Empty bodies are treated as
// "no data" -- the caller decides whether that is an error.
func decodeInto(body string, dst interface{}) error {
	if body == "" {
		return errors.New("empty server response")
	}
	if err := json.Unmarshal([]byte(body), dst); err != nil {
		return errors.New("schema mismatch: " + err.Error())
	}
	return nil
}

// ---- Image profiles ---------------------------------------------------------

// GetProfiles -> GET /api/image/profiles.
func (r *RestClient) GetProfiles(timeoutMs uint32) ([]ProfileConfig, error) {
	body, err := r.Get("/api/image/profiles", timeoutMs)
	if err != nil {
		return nil, err
	}
	var out []ProfileConfig
	return out, decodeInto(body, &out)
}

// GetProfile -> GET /api/image/profiles?id=<id>.  The server may return
// an array even for a single id, so the typed result is a slice.
func (r *RestClient) GetProfile(id int, timeoutMs uint32) ([]ProfileConfig, error) {
	body, err := r.Get(fmt.Sprintf("/api/image/profiles?id=%d", id),
		timeoutMs)
	if err != nil {
		return nil, err
	}
	var out []ProfileConfig
	return out, decodeInto(body, &out)
}

// CreateProfile -> POST /api/image/profiles.
func (r *RestClient) CreateProfile(p ProfileConfig, timeoutMs uint32) (ProfileConfig, error) {
	body, err := r.postJSON("/api/image/profiles", p, timeoutMs)
	if err != nil {
		return ProfileConfig{}, err
	}
	var out ProfileConfig
	return out, decodeInto(body, &out)
}

// UpdateProfileById -> PUT /api/image/profiles/{id}.
//
// Warning: sends a full ProfileConfig document.  The daemon rejects
// full-document PUT on this endpoint (HTTP 500).  Use PatchJSON with
// only the fields you want to change.
func (r *RestClient) UpdateProfileById(id int, p ProfileConfig, timeoutMs uint32) (ProfileConfig, error) {
	body, err := r.putJSON(
		fmt.Sprintf("/api/image/profiles/%d", id), p, timeoutMs)
	if err != nil {
		return ProfileConfig{}, err
	}
	var out ProfileConfig
	return out, decodeInto(body, &out)
}

// UpdateProfiles -> PUT /api/image/profiles (bulk update).
//
// Warning: same partial-PUT caveat as UpdateProfileById.
func (r *RestClient) UpdateProfiles(ps []ProfileConfig, timeoutMs uint32) (ProfileConfig, error) {
	body, err := r.putJSON("/api/image/profiles", ps, timeoutMs)
	if err != nil {
		return ProfileConfig{}, err
	}
	var out ProfileConfig
	return out, decodeInto(body, &out)
}

// DeleteProfile -> DELETE /api/image/profiles?id=<id>.  Returns the
// server's raw response body unchanged.
func (r *RestClient) DeleteProfile(id int, timeoutMs uint32) (string, error) {
	return r.Delete(fmt.Sprintf("/api/image/profiles?id=%d", id), timeoutMs)
}

// ---- Equipment misc / volatile info ----------------------------------------

func (r *RestClient) GetVolatileInfo(timeoutMs uint32) (MiscVolatile, error) {
	body, err := r.Get("/api/equipment/misc/readonly/volatile", timeoutMs)
	if err != nil {
		return MiscVolatile{}, err
	}
	var out MiscVolatile
	return out, decodeInto(body, &out)
}

func (r *RestClient) GetMisc(timeoutMs uint32) (Misc, error) {
	body, err := r.Get("/api/equipment/misc", timeoutMs)
	if err != nil {
		return Misc{}, err
	}
	var out Misc
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetMisc(cfg Misc, timeoutMs uint32) (Misc, error) {
	body, err := r.putJSON("/api/equipment/misc", cfg, timeoutMs)
	if err != nil {
		return Misc{}, err
	}
	var out Misc
	return out, decodeInto(body, &out)
}

// ---- OCR --------------------------------------------------------------------

func (r *RestClient) GetOcrConfig(timeoutMs uint32) (OcrConfig, error) {
	body, err := r.Get("/api/equipment/ocr", timeoutMs)
	if err != nil {
		return OcrConfig{}, err
	}
	var out OcrConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetOcrConfig(cfg OcrConfig, timeoutMs uint32) (OcrConfig, error) {
	body, err := r.putJSON("/api/equipment/ocr", cfg, timeoutMs)
	if err != nil {
		return OcrConfig{}, err
	}
	var out OcrConfig
	return out, decodeInto(body, &out)
}

// ---- Analytics --------------------------------------------------------------

func (r *RestClient) GetAnalyticsConfig(timeoutMs uint32) (AnalyticsConfig, error) {
	body, err := r.Get("/api/equipment/analytics", timeoutMs)
	if err != nil {
		return AnalyticsConfig{}, err
	}
	var out AnalyticsConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetAnalyticsConfig(cfg AnalyticsConfig, timeoutMs uint32) (AnalyticsConfig, error) {
	body, err := r.putJSON("/api/equipment/analytics", cfg, timeoutMs)
	if err != nil {
		return AnalyticsConfig{}, err
	}
	var out AnalyticsConfig
	return out, decodeInto(body, &out)
}

// ---- Classifier -------------------------------------------------------------

func (r *RestClient) GetClassifierConfig(timeoutMs uint32) (ClassifierConfig, error) {
	body, err := r.Get("/api/equipment/classifier", timeoutMs)
	if err != nil {
		return ClassifierConfig{}, err
	}
	var out ClassifierConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetClassifierConfig(cfg ClassifierConfig, timeoutMs uint32) (ClassifierConfig, error) {
	body, err := r.putJSON("/api/equipment/classifier", cfg, timeoutMs)
	if err != nil {
		return ClassifierConfig{}, err
	}
	var out ClassifierConfig
	return out, decodeInto(body, &out)
}

// ---- AutoFocus --------------------------------------------------------------

func (r *RestClient) GetAutoFocus(timeoutMs uint32) (AutoFocus, error) {
	body, err := r.Get("/api/equipment/autofocus", timeoutMs)
	if err != nil {
		return AutoFocus{}, err
	}
	var out AutoFocus
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetAutoFocus(cfg AutoFocus, timeoutMs uint32) (AutoFocus, error) {
	body, err := r.putJSON("/api/equipment/autofocus", cfg, timeoutMs)
	if err != nil {
		return AutoFocus{}, err
	}
	var out AutoFocus
	return out, decodeInto(body, &out)
}

// ---- Video streams ----------------------------------------------------------

func (r *RestClient) GetStreamConfig(timeoutMs uint32) (StreamConfig, error) {
	body, err := r.Get("/api/video/streams", timeoutMs)
	if err != nil {
		return StreamConfig{}, err
	}
	var out StreamConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetStreamConfig(cfg StreamConfig, timeoutMs uint32) (StreamConfig, error) {
	body, err := r.putJSON("/api/video/streams", cfg, timeoutMs)
	if err != nil {
		return StreamConfig{}, err
	}
	var out StreamConfig
	return out, decodeInto(body, &out)
}

// ---- Lanes ------------------------------------------------------------------

func (r *RestClient) GetLanesConfig(timeoutMs uint32) (LanesConfig, error) {
	body, err := r.Get("/api/equipment/lanes", timeoutMs)
	if err != nil {
		return LanesConfig{}, err
	}
	var out LanesConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetLanesConfig(cfg LanesConfig, timeoutMs uint32) (LanesConfig, error) {
	body, err := r.putJSON("/api/equipment/lanes", cfg, timeoutMs)
	if err != nil {
		return LanesConfig{}, err
	}
	var out LanesConfig
	return out, decodeInto(body, &out)
}

// ---- ITSCAM PRO server ------------------------------------------------------

func (r *RestClient) GetItscamproConfig(timeoutMs uint32) (ItscamproConfig, error) {
	body, err := r.Get("/api/equipment/servers/itscampro", timeoutMs)
	if err != nil {
		return ItscamproConfig{}, err
	}
	var out ItscamproConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetItscamproConfig(cfg ItscamproConfig, timeoutMs uint32) (ItscamproConfig, error) {
	body, err := r.putJSON("/api/equipment/servers/itscampro", cfg, timeoutMs)
	if err != nil {
		return ItscamproConfig{}, err
	}
	var out ItscamproConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) GetItscamproStatus(timeoutMs uint32) (ItscamproStatus, error) {
	body, err := r.Get("/api/equipment/servers/itscampro/status", timeoutMs)
	if err != nil {
		return ItscamproStatus{}, err
	}
	var out ItscamproStatus
	return out, decodeInto(body, &out)
}

// ---- Image sign -------------------------------------------------------------

func (r *RestClient) GetImageSignConfig(timeoutMs uint32) (ImageSignConfig, error) {
	body, err := r.Get("/api/equipment/imageSign", timeoutMs)
	if err != nil {
		return ImageSignConfig{}, err
	}
	var out ImageSignConfig
	return out, decodeInto(body, &out)
}

// ---- FTP --------------------------------------------------------------------

func (r *RestClient) GetFtpConfig(timeoutMs uint32) (FTPConfig, error) {
	body, err := r.Get("/api/equipment/servers/ftp", timeoutMs)
	if err != nil {
		return FTPConfig{}, err
	}
	var out FTPConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetFtpConfig(cfg FTPConfig, timeoutMs uint32) (FTPConfig, error) {
	body, err := r.putJSON("/api/equipment/servers/ftp", cfg, timeoutMs)
	if err != nil {
		return FTPConfig{}, err
	}
	var out FTPConfig
	return out, decodeInto(body, &out)
}

// ---- Lince ------------------------------------------------------------------

func (r *RestClient) GetLinceConfig(timeoutMs uint32) (LinceConfig, error) {
	body, err := r.Get("/api/equipment/servers/lince", timeoutMs)
	if err != nil {
		return LinceConfig{}, err
	}
	var out LinceConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetLinceConfig(cfg LinceConfig, timeoutMs uint32) (LinceConfig, error) {
	body, err := r.putJSON("/api/equipment/servers/lince", cfg, timeoutMs)
	if err != nil {
		return LinceConfig{}, err
	}
	var out LinceConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) GetLinceStatus(timeoutMs uint32) (LinceStatus, error) {
	body, err := r.Get("/api/equipment/servers/lince/status", timeoutMs)
	if err != nil {
		return LinceStatus{}, err
	}
	var out LinceStatus
	return out, decodeInto(body, &out)
}

// ---- Vehicle indicator -----------------------------------------------------

func (r *RestClient) GetVehicleIndicatorConfig(timeoutMs uint32) (VehicleIndicatorConfig, error) {
	body, err := r.Get("/api/equipment/vehicleIndicator", timeoutMs)
	if err != nil {
		return VehicleIndicatorConfig{}, err
	}
	var out VehicleIndicatorConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetVehicleIndicatorConfig(cfg VehicleIndicatorConfig, timeoutMs uint32) (VehicleIndicatorConfig, error) {
	body, err := r.putJSON("/api/equipment/vehicleIndicator", cfg, timeoutMs)
	if err != nil {
		return VehicleIndicatorConfig{}, err
	}
	var out VehicleIndicatorConfig
	return out, decodeInto(body, &out)
}

// ---- Output protocols ------------------------------------------------------

func (r *RestClient) GetProtocolsConfig(timeoutMs uint32) (ProtocolsConfig, error) {
	body, err := r.Get("/api/equipment/servers/protocols", timeoutMs)
	if err != nil {
		return ProtocolsConfig{}, err
	}
	var out ProtocolsConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetProtocolsConfig(cfg ProtocolsConfig, timeoutMs uint32) (ProtocolsConfig, error) {
	body, err := r.putJSON("/api/equipment/servers/protocols", cfg, timeoutMs)
	if err != nil {
		return ProtocolsConfig{}, err
	}
	var out ProtocolsConfig
	return out, decodeInto(body, &out)
}

// ---- Profile transitioner --------------------------------------------------

func (r *RestClient) GetProfileTransitioner(timeoutMs uint32) (ProfileTransitioner, error) {
	body, err := r.Get("/api/equipment/transitioner", timeoutMs)
	if err != nil {
		return ProfileTransitioner{}, err
	}
	var out ProfileTransitioner
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetProfileTransitioner(cfg ProfileTransitioner, timeoutMs uint32) (ProfileTransitioner, error) {
	body, err := r.putJSON("/api/equipment/transitioner", cfg, timeoutMs)
	if err != nil {
		return ProfileTransitioner{}, err
	}
	var out ProfileTransitioner
	return out, decodeInto(body, &out)
}

// ---- I/O ports -------------------------------------------------------------

func (r *RestClient) GetIoPorts(timeoutMs uint32) ([]IoConfig, error) {
	body, err := r.Get("/api/equipment/ioPorts", timeoutMs)
	if err != nil {
		return nil, err
	}
	var out []IoConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetIoPorts(ports []IoConfig, timeoutMs uint32) ([]IoConfig, error) {
	body, err := r.putJSON("/api/equipment/ioPorts", ports, timeoutMs)
	if err != nil {
		return nil, err
	}
	var out []IoConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) GetIoPort(id int, timeoutMs uint32) (IoConfig, error) {
	body, err := r.Get(fmt.Sprintf("/api/equipment/ioPorts/%d", id), timeoutMs)
	if err != nil {
		return IoConfig{}, err
	}
	var out IoConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetIoPort(id int, port IoConfig, timeoutMs uint32) (IoConfig, error) {
	body, err := r.putJSON(
		fmt.Sprintf("/api/equipment/ioPorts/%d", id), port, timeoutMs)
	if err != nil {
		return IoConfig{}, err
	}
	var out IoConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) GetIoBasic(timeoutMs uint32) ([]IoBasic, error) {
	body, err := r.Get("/api/equipment/ioBasic", timeoutMs)
	if err != nil {
		return nil, err
	}
	var out []IoBasic
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetIoBasic(ports []IoBasic, timeoutMs uint32) ([]IoBasic, error) {
	body, err := r.putJSON("/api/equipment/ioBasic", ports, timeoutMs)
	if err != nil {
		return nil, err
	}
	var out []IoBasic
	return out, decodeInto(body, &out)
}

// ---- REST API client (webhook) servers --------------------------------------

func (r *RestClient) GetRestApiClientConfig(id int, timeoutMs uint32) (RESTAPIClientConfig, error) {
	body, err := r.Get(fmt.Sprintf(
		"/api/equipment/servers/restapiclient/%d/config", id), timeoutMs)
	if err != nil {
		return RESTAPIClientConfig{}, err
	}
	var out RESTAPIClientConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) SetRestApiClientConfig(id int, cfg RESTAPIClientConfig, timeoutMs uint32) (RESTAPIClientConfig, error) {
	body, err := r.putJSON(fmt.Sprintf(
		"/api/equipment/servers/restapiclient/%d/config", id), cfg, timeoutMs)
	if err != nil {
		return RESTAPIClientConfig{}, err
	}
	var out RESTAPIClientConfig
	return out, decodeInto(body, &out)
}

func (r *RestClient) GetRestApiClientStatus(id int, timeoutMs uint32) (RESTAPIClientStatus, error) {
	body, err := r.Get(fmt.Sprintf(
		"/api/equipment/servers/restapiclient/%d/status", id), timeoutMs)
	if err != nil {
		return RESTAPIClientStatus{}, err
	}
	var out RESTAPIClientStatus
	return out, decodeInto(body, &out)
}

// ---- Licenses --------------------------------------------------------------

func (r *RestClient) GetLicenses(timeoutMs uint32) (Licenses, error) {
	body, err := r.Get("/api/system/licenses", timeoutMs)
	if err != nil {
		return Licenses{}, err
	}
	var out Licenses
	return out, decodeInto(body, &out)
}

// ============================================================================
//  Shared helpers (also used by CgiClient)
// ============================================================================

// takeString consumes an ITSCAM_String*, returning the UTF-8 body and
// freeing the native object.
func takeString(h *C.ITSCAM_String) string {
	if h == nil {
		return ""
	}
	defer C.ITSCAM_String_destroy(h)
	ptr := C.ITSCAM_String_data(h)
	if ptr == nil {
		return ""
	}
	return C.GoString(ptr)
}

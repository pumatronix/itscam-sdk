package itscam

/*
#cgo !static LDFLAGS: -litscam_sdk
#cgo static,linux LDFLAGS: ${SRCDIR}/../../../core/build/linux/libitscam_sdk.a -lstdc++ -lpthread -lm
#cgo static,windows CFLAGS: -DITSCAM_SDK_STATIC
#cgo static,windows LDFLAGS: ${SRCDIR}/../../../core/build/win-x64/libitscam_sdk_static.a -lws2_32 -lbcrypt -lcrypt32 -lstdc++ -lm -static
#include <stdlib.h>
#include "../../../core/c_api/itscam_sdk_c.h"

// Forward declarations for Go callbacks (without const to match CGo-generated signatures)
extern void goCaptureCallback(ITSCAM_CaptureResult* result, void* userdata);
extern void goSnapshotCallback(ITSCAM_CaptureResult* result, void* userdata);
extern void goDisconnectCallback(char* reason, void* userdata);
extern void goConnectionStateCallback(int state, char* reason, void* userdata);
extern void goLogCallback(int level, char* message, void* userdata);

// Wrapper functions using the correct C API names
static inline void setCaptureCallback(ITSCAM_Client* client, void* userdata) {
    ITSCAM_Client_onTriggerImage(client, (ITSCAM_CaptureCallback)goCaptureCallback, userdata);
}

static inline void setSnapshotCallback(ITSCAM_Client* client, void* userdata) {
    ITSCAM_Client_onSnapshotImage(client, (ITSCAM_CaptureCallback)goSnapshotCallback, userdata);
}

static inline void setDisconnectCallback(ITSCAM_Client* client, void* userdata) {
    ITSCAM_Client_onDisconnect(client, (ITSCAM_DisconnectCallback)goDisconnectCallback, userdata);
}

static inline void setConnectionStateCallback(ITSCAM_Client* client, void* userdata) {
    ITSCAM_Client_onConnectionState(client, (ITSCAM_ConnectionStateCallback)goConnectionStateCallback, userdata);
}

static inline void setLogCallback(ITSCAM_Client* client, void* userdata) {
    ITSCAM_Client_onLog(client, (ITSCAM_LogCallback)goLogCallback, userdata);
}
*/
import "C"
import (
	"sync"
	"unsafe"
)

// clientCallbacks holds the Go callbacks of a Client. It does not refer to the
// Client, so the registry does not keep the Client reachable.
type clientCallbacks struct {
	mu              sync.Mutex
	capture         func(*CaptureResult)
	snapshot        func(*CaptureResult)
	disconnect      func(string)
	connectionState func(ConnectionState, string)
	log             func(LogLevel, string)
}

// Global registry to map C pointers back to Go callbacks
var (
	callbackRegistry = make(map[uintptr]*clientCallbacks)
	callbackMu       sync.RWMutex
)

func registerCallbacks(cbs *clientCallbacks, handle *C.ITSCAM_Client) {
	callbackMu.Lock()
	defer callbackMu.Unlock()
	callbackRegistry[uintptr(unsafe.Pointer(handle))] = cbs
}

func unregisterCallbacks(handle *C.ITSCAM_Client) {
	callbackMu.Lock()
	defer callbackMu.Unlock()
	key := uintptr(unsafe.Pointer(handle))
	delete(callbackRegistry, key)
}

func getCallbacks(userdata unsafe.Pointer) *clientCallbacks {
	callbackMu.RLock()
	defer callbackMu.RUnlock()
	return callbackRegistry[uintptr(userdata)]
}

func captureResultFromC(result *C.ITSCAM_CaptureResult) *CaptureResult {
	if result == nil {
		return &CaptureResult{}
	}

	info := C.ITSCAM_CaptureResult_getInfo(result)

	plateCount := int(C.ITSCAM_CaptureResult_getPlateCount(result))
	plates := make([]string, 0, plateCount)
	for i := 0; i < plateCount; i++ {
		plate := C.ITSCAM_CaptureResult_getPlate(result, C.size_t(i))
		if plate != nil {
			plates = append(plates, C.GoString(plate))
		}
	}

	var jpegSize C.size_t
	jpegPtr := C.ITSCAM_CaptureResult_getJpeg(result, &jpegSize)
	var jpegData []byte
	if jpegPtr != nil && jpegSize > 0 {
		jpegData = C.GoBytes(unsafe.Pointer(jpegPtr), C.int(jpegSize))
	}

	return &CaptureResult{
		FrameInfo: FrameInfo{
			RequestID:      uint64(info.requestId),
			FrameCount:     uint64(info.frameCount),
			MultiExpIndex:  int(info.multiExpIndex),
			MultiExpLength: int(info.multiExpLength),
			Shutter:        int(info.shutter),
			Gain:           float32(info.gain),
			Width:          int(info.width),
			Height:         int(info.height),
			Timestamp: Timestamp{
				Year:           int(info.timestamp.year),
				Month:          int(info.timestamp.month),
				Day:            int(info.timestamp.day),
				Hour:           int(info.timestamp.hour),
				Minute:         int(info.timestamp.minute),
				Second:         int(info.timestamp.second),
				Millisecond:    int(info.timestamp.millisecond),
				TimezoneOffset: int(info.timestamp.timezone_offset),
			},
		},
		JpegData: jpegData,
		Plates:   plates,
	}
}

func takeCaptureResults(array *C.ITSCAM_CaptureResultArray) []CaptureResult {
	if array == nil {
		return nil
	}

	n := int(C.ITSCAM_CaptureResultArray_size(array))
	results := make([]CaptureResult, 0, n)
	for i := 0; i < n; i++ {
		ptr := C.ITSCAM_CaptureResultArray_get(array, C.size_t(i))
		if ptr == nil {
			continue
		}
		if cr := captureResultFromC(ptr); cr != nil {
			results = append(results, *cr)
		}
	}
	C.ITSCAM_CaptureResultArray_destroy(array)
	return results
}

func takeByteArray(array *C.ITSCAM_ByteArray) []byte {
	if array == nil {
		return nil
	}

	n := int(C.ITSCAM_ByteArray_size(array))
	if n <= 0 {
		C.ITSCAM_ByteArray_destroy(array)
		return nil
	}

	data := C.ITSCAM_ByteArray_data(array)
	out := C.GoBytes(unsafe.Pointer(data), C.int(n))
	C.ITSCAM_ByteArray_destroy(array)
	return out
}

func takeProfiles(array *C.ITSCAM_ProfileArray) []ProfileInfo {
	if array == nil {
		return nil
	}

	n := int(C.ITSCAM_ProfileArray_size(array))
	profiles := make([]ProfileInfo, 0, n)
	for i := 0; i < n; i++ {
		var info C.ITSCAM_ProfileInfo
		if C.ITSCAM_ProfileArray_get(array, C.size_t(i), &info) == 0 {
			continue
		}
		profiles = append(profiles, ProfileInfo{
			ID:          uint32(info.id),
			Name:        C.GoString(info.name),
			Description: C.GoString(info.description),
			IsActive:    info.isActive != 0,
		})
	}
	C.ITSCAM_ProfileArray_destroy(array)
	return profiles
}

// installCallbacks installs every native trampoline, while no callback can be
// running yet (from NewClient): the setters then only swap the Go functions.
// A native setter waits for the callback in progress, so calling one later
// could deadlock with a callback that calls a method of this client.
func installCallbacks(cbs *clientCallbacks, handle *C.ITSCAM_Client) {
	registerCallbacks(cbs, handle)
	key := unsafe.Pointer(handle) // userdata: the registry key
	C.setCaptureCallback(handle, key)
	C.setSnapshotCallback(handle, key)
	C.setDisconnectCallback(handle, key)
	C.setConnectionStateCallback(handle, key)
	C.setLogCallback(handle, key)
}

// callbackOf reads a callback field of cbs under its mutex
func callbackOf[F any](cbs *clientCallbacks, field *F) F {
	cbs.mu.Lock()
	defer cbs.mu.Unlock()
	return *field
}

// SetCaptureCallback sets the callback for capture events.
func (c *Client) SetCaptureCallback(callback func(*CaptureResult)) {
	c.cbs.mu.Lock()
	defer c.cbs.mu.Unlock()
	c.cbs.capture = callback
}

// SetDisconnectCallback sets the callback for disconnect events.
func (c *Client) SetDisconnectCallback(callback func(string)) {
	c.cbs.mu.Lock()
	defer c.cbs.mu.Unlock()
	c.cbs.disconnect = callback
}

// SetConnectionStateCallback sets the callback for connection state changes.
func (c *Client) SetConnectionStateCallback(callback func(ConnectionState, string)) {
	c.cbs.mu.Lock()
	defer c.cbs.mu.Unlock()
	c.cbs.connectionState = callback
}

// SetLogCallback sets the callback for log messages.
func (c *Client) SetLogCallback(callback func(LogLevel, string)) {
	c.cbs.mu.Lock()
	defer c.cbs.mu.Unlock()
	c.cbs.log = callback
}

// SetSnapshotCallback sets the callback for snapshot events.
func (c *Client) SetSnapshotCallback(callback func(*CaptureResult)) {
	c.cbs.mu.Lock()
	defer c.cbs.mu.Unlock()
	c.cbs.snapshot = callback
}

//export goCaptureCallback
func goCaptureCallback(result *C.ITSCAM_CaptureResult, userdata unsafe.Pointer) {
	cbs := getCallbacks(userdata)
	if cbs == nil {
		return
	}
	if cb := callbackOf(cbs, &cbs.capture); cb != nil {
		cb(captureResultFromC(result))
	}
}

//export goSnapshotCallback
func goSnapshotCallback(result *C.ITSCAM_CaptureResult, userdata unsafe.Pointer) {
	cbs := getCallbacks(userdata)
	if cbs == nil {
		return
	}
	if cb := callbackOf(cbs, &cbs.snapshot); cb != nil {
		cb(captureResultFromC(result))
	}
}

//export goDisconnectCallback
func goDisconnectCallback(reason *C.char, userdata unsafe.Pointer) {
	cbs := getCallbacks(userdata)
	if cbs == nil {
		return
	}
	if cb := callbackOf(cbs, &cbs.disconnect); cb != nil {
		cb(C.GoString(reason))
	}
}

//export goConnectionStateCallback
func goConnectionStateCallback(state C.int, reason *C.char, userdata unsafe.Pointer) {
	cbs := getCallbacks(userdata)
	if cbs == nil {
		return
	}
	if cb := callbackOf(cbs, &cbs.connectionState); cb != nil {
		cb(ConnectionState(state), C.GoString(reason))
	}
}

//export goLogCallback
func goLogCallback(level C.int, message *C.char, userdata unsafe.Pointer) {
	cbs := getCallbacks(userdata)
	if cbs == nil {
		return
	}
	if cb := callbackOf(cbs, &cbs.log); cb != nil {
		cb(LogLevel(level), C.GoString(message))
	}
}

package itscam

/*
#cgo !static LDFLAGS: -litscam_sdk
#cgo static,linux LDFLAGS: ${SRCDIR}/../../../core/build/linux/libitscam_sdk.a -lstdc++ -lpthread -lm
#cgo static,windows CFLAGS: -DITSCAM_SDK_STATIC
#cgo static,windows LDFLAGS: ${SRCDIR}/../../../core/build/win-x64/libitscam_sdk_static.a -lws2_32 -lstdc++ -lm -static
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

// Global registry to map C pointers back to Go clients
var (
	callbackRegistry = make(map[uintptr]*Client)
	callbackMu       sync.RWMutex
)

func registerClient(c *Client) uintptr {
	callbackMu.Lock()
	defer callbackMu.Unlock()
	key := uintptr(unsafe.Pointer(c.handle))
	callbackRegistry[key] = c
	return key
}

func unregisterClient(c *Client) {
	callbackMu.Lock()
	defer callbackMu.Unlock()
	key := uintptr(unsafe.Pointer(c.handle))
	delete(callbackRegistry, key)
}

func getClient(userdata unsafe.Pointer) *Client {
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

// SetCaptureCallback sets the callback for capture events.
func (c *Client) SetCaptureCallback(callback func(*CaptureResult)) {
	c.mu.Lock()
	defer c.mu.Unlock()
	
	if c.closed {
		return
	}
	
	c.captureCallback = callback
	
	if callback != nil {
		key := registerClient(c)
		C.setCaptureCallback(c.handle, unsafe.Pointer(key))
	} else {
		C.ITSCAM_Client_onTriggerImage(c.handle, nil, nil)
	}
}

// SetDisconnectCallback sets the callback for disconnect events.
func (c *Client) SetDisconnectCallback(callback func(string)) {
	c.mu.Lock()
	defer c.mu.Unlock()
	
	if c.closed {
		return
	}
	
	c.disconnectCallback = callback
	
	if callback != nil {
		key := registerClient(c)
		C.setDisconnectCallback(c.handle, unsafe.Pointer(key))
	} else {
		C.ITSCAM_Client_onDisconnect(c.handle, nil, nil)
	}
}

// SetConnectionStateCallback sets the callback for connection state changes.
func (c *Client) SetConnectionStateCallback(callback func(ConnectionState, string)) {
	c.mu.Lock()
	defer c.mu.Unlock()
	
	if c.closed {
		return
	}
	
	c.connectionStateCallback = callback
	
	if callback != nil {
		key := registerClient(c)
		C.setConnectionStateCallback(c.handle, unsafe.Pointer(key))
	} else {
		C.ITSCAM_Client_onConnectionState(c.handle, nil, nil)
	}
}

// SetLogCallback sets the callback for log messages.
func (c *Client) SetLogCallback(callback func(LogLevel, string)) {
	c.mu.Lock()
	defer c.mu.Unlock()
	
	if c.closed {
		return
	}
	
	c.logCallback = callback
	
	if callback != nil {
		key := registerClient(c)
		C.setLogCallback(c.handle, unsafe.Pointer(key))
	} else {
		C.ITSCAM_Client_onLog(c.handle, nil, nil)
	}
}

// SetSnapshotCallback sets the callback for snapshot events.
func (c *Client) SetSnapshotCallback(callback func(*CaptureResult)) {
	c.mu.Lock()
	defer c.mu.Unlock()
	
	if c.closed {
		return
	}
	
	c.snapshotCallback = callback
	
	if callback != nil {
		key := registerClient(c)
		C.setSnapshotCallback(c.handle, unsafe.Pointer(key))
	} else {
		C.ITSCAM_Client_onSnapshotImage(c.handle, nil, nil)
	}
}

//export goCaptureCallback
func goCaptureCallback(result *C.ITSCAM_CaptureResult, userdata unsafe.Pointer) {
	client := getClient(userdata)
	if client == nil || client.captureCallback == nil {
		return
	}
	client.captureCallback(captureResultFromC(result))
}

//export goSnapshotCallback
func goSnapshotCallback(result *C.ITSCAM_CaptureResult, userdata unsafe.Pointer) {
	client := getClient(userdata)
	if client == nil || client.snapshotCallback == nil {
		return
	}
	client.snapshotCallback(captureResultFromC(result))
}

//export goDisconnectCallback
func goDisconnectCallback(reason *C.char, userdata unsafe.Pointer) {
	client := getClient(userdata)
	if client == nil || client.disconnectCallback == nil {
		return
	}
	client.disconnectCallback(C.GoString(reason))
}

//export goConnectionStateCallback
func goConnectionStateCallback(state C.int, reason *C.char, userdata unsafe.Pointer) {
	client := getClient(userdata)
	if client == nil || client.connectionStateCallback == nil {
		return
	}
	client.connectionStateCallback(ConnectionState(state), C.GoString(reason))
}

//export goLogCallback
func goLogCallback(level C.int, message *C.char, userdata unsafe.Pointer) {
	client := getClient(userdata)
	if client == nil || client.logCallback == nil {
		return
	}
	client.logCallback(LogLevel(level), C.GoString(message))
}

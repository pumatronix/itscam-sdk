// Package itscam provides a Go client for ITSCAM cameras.
//
// This package wraps the native ITSCAM SDK library using cgo.
// The native library (libitscam_sdk.so or itscam_sdk.dll) must be
// installed on the system.
//
// # Quick Start
//
//	client, err := itscam.NewClient()
//	if err != nil {
//	    log.Fatal(err)
//	}
//	defer client.Close()
//
//	if err := client.Connect("192.168.1.100", 60000, 5*time.Second); err != nil {
//	    log.Fatal(err)
//	}
//
//	if err := client.Authenticate("password", 5*time.Second); err != nil {
//	    log.Fatal(err)
//	}
package itscam

/*
#cgo !static LDFLAGS: -litscam_sdk
#cgo static,linux LDFLAGS: ${SRCDIR}/../../../core/build/linux/libitscam_sdk.a -lstdc++ -lpthread -lm
#cgo static,windows CFLAGS: -DITSCAM_SDK_STATIC
#cgo static,windows LDFLAGS: ${SRCDIR}/../../../core/build/win-x64/libitscam_sdk_static.a -lws2_32 -lbcrypt -lcrypt32 -lstdc++ -lm -static
#include <stdlib.h>
#include <stdbool.h>
#include "../../../core/c_api/itscam_sdk_c.h"
*/
import "C"
import (
	"errors"
	"fmt"
	"runtime"
	"sync"
	"time"
	"unsafe"
)

// Client represents a connection to an ITSCAM camera.
//
// The SDK runs callbacks on its own threads, and a callback may call methods
// of its client while another goroutine is inside one; ITSCAM_Client_destroy()
// waits for the callback in progress. So mu only guards the fields below and
// is never held during a native call: calls on one client may run
// concurrently, as in the other bindings.
type Client struct {
	mu     sync.Mutex
	handle *C.ITSCAM_Client
	closed bool
	calls  int // native calls in progress, see begin()

	// Connection info (for reconnection/info)
	address string
	port    uint16

	// Read by the native callbacks, through a registry that must not keep the
	// client reachable: a client dropped without Close is then finalized.
	cbs *clientCallbacks
}

var errClosed = errors.New("client is closed")

// NewClient creates a new ITSCAM client.
func NewClient() (*Client, error) {
	handle := C.ITSCAM_Client_create()
	if handle == nil {
		return nil, errors.New("failed to create client")
	}

	client := &Client{handle: handle, cbs: &clientCallbacks{}}
	installCallbacks(client.cbs, handle)
	runtime.SetFinalizer(client, (*Client).Close)
	return client, nil
}

// begin starts a native call: it returns the handle, or nil if the client is
// closed. Call c.end() once the native call has returned.
func (c *Client) begin() *C.ITSCAM_Client {
	c.mu.Lock()
	defer c.mu.Unlock()
	if c.closed {
		return nil
	}
	c.calls++
	return c.handle
}

// end finishes a native call started by begin(). The last one to end after
// Close destroys the handle, in another goroutine: a callback may be waiting
// for this call, and ITSCAM_Client_destroy() waits for the callbacks.
func (c *Client) end() {
	c.mu.Lock()
	c.calls--
	handle := c.takeHandleIfUnused()
	c.mu.Unlock()
	if handle != nil {
		go func() { C.ITSCAM_Client_destroy(handle) }()
	}
}

// takeHandleIfUnused returns the handle once the client is closed and no
// native call uses it, or nil. Called with mu held.
func (c *Client) takeHandleIfUnused() *C.ITSCAM_Client {
	if !c.closed || c.calls > 0 {
		return nil
	}
	handle := c.handle
	c.handle = nil
	return handle
}

// Close releases the client resources. It does not wait for native calls in
// progress, such as the one whose callback calls Close: the last of them
// releases the client.
func (c *Client) Close() error {
	c.mu.Lock()
	if c.closed {
		c.mu.Unlock()
		return nil
	}
	c.closed = true
	// Callbacks still arriving are dropped. Unregistered under mu: once it
	// is released, the last native call may destroy the handle, and a new
	// client may then register the same address.
	unregisterCallbacks(c.handle)
	handle := c.takeHandleIfUnused()
	runtime.SetFinalizer(c, nil)
	c.mu.Unlock()

	// ITSCAM_Client_destroy() waits for the callbacks in progress: those may
	// call methods of this client, which find it closed since the mutex is
	// not held here.
	if handle != nil {
		C.ITSCAM_Client_destroy(handle)
	}
	return nil
}

// Connect establishes a connection to the camera.
func (c *Client) Connect(address string, port uint16, timeout time.Duration) error {
	h := c.begin()
	if h == nil {
		return errClosed
	}
	defer c.end()

	caddr := C.CString(address)
	defer C.free(unsafe.Pointer(caddr))

	c.mu.Lock()
	c.address = address
	c.port = port
	c.mu.Unlock()

	result := C.ITSCAM_Client_connect(
		h,
		caddr,
		C.uint16_t(port),
		C.uint32_t(timeout.Milliseconds()),
		nil) // no auto-reconnect config
	return errorFromCode(int(result))
}

// ConnectWithReconnect establishes a connection with auto-reconnect enabled.
func (c *Client) ConnectWithReconnect(address string, port uint16, timeout time.Duration, reconnect AutoReconnectConfig) error {
	h := c.begin()
	if h == nil {
		return errClosed
	}
	defer c.end()

	caddr := C.CString(address)
	defer C.free(unsafe.Pointer(caddr))

	c.mu.Lock()
	c.address = address
	c.port = port
	c.mu.Unlock()

	var cReconnect C.ITSCAM_AutoReconnectConfig
	if reconnect.Enabled {
		cReconnect.enabled = 1
	} else {
		cReconnect.enabled = 0
	}
	cReconnect.intervalMs = C.uint32_t(reconnect.Interval.Milliseconds())
	cReconnect.maxRetries = C.uint32_t(reconnect.MaxAttempts)

	result := C.ITSCAM_Client_connect(
		h,
		caddr,
		C.uint16_t(port),
		C.uint32_t(timeout.Milliseconds()),
		&cReconnect)
	return errorFromCode(int(result))
}

// Disconnect closes the connection to the camera.
func (c *Client) Disconnect() {
	h := c.begin()
	if h == nil {
		return
	}
	defer c.end()
	C.ITSCAM_Client_disconnect(h)
}

// IsConnected returns whether the client is connected.
func (c *Client) IsConnected() bool {
	h := c.begin()
	if h == nil {
		return false
	}
	defer c.end()
	return C.ITSCAM_Client_isConnected(h) != 0
}

// Authenticate authenticates with the camera.
func (c *Client) Authenticate(password string, timeout time.Duration) error {
	h := c.begin()
	if h == nil {
		return errClosed
	}
	defer c.end()

	cpass := C.CString(password)
	defer C.free(unsafe.Pointer(cpass))

	result := C.ITSCAM_Client_authenticate(h, cpass, C.uint32_t(timeout.Milliseconds()))
	return errorFromCode(int(result))
}

// Subscribe subscribes to camera events.
func (c *Client) Subscribe(events EventSubscription, timeout time.Duration) error {
	h := c.begin()
	if h == nil {
		return errClosed
	}
	defer c.end()

	var cEvents C.ITSCAM_EventSubscription
	if events.Pipeline {
		cEvents.pipeline = 1
	}
	if events.TriggerMetadata {
		cEvents.triggerMetadata = 1
	}
	if events.TriggerImage {
		cEvents.triggerImage = 1
	}
	if events.SnapshotMetadata {
		cEvents.snapshotMetadata = 1
	}
	if events.SnapshotImage {
		cEvents.snapshotImage = 1
	}
	if events.PreviewMetadata {
		cEvents.previewMetadata = 1
	}
	if events.PreviewImage {
		cEvents.previewImage = 1
	}
	if events.GPIO {
		cEvents.gpio = 1
	}
	if events.Serial1 {
		cEvents.serial1 = 1
	}
	if events.Serial2 {
		cEvents.serial2 = 1
	}

	result := C.ITSCAM_Client_subscribe(h, &cEvents, C.uint32_t(timeout.Milliseconds()))
	return errorFromCode(int(result))
}

// SubscribeCaptures subscribes to capture events using the SDK high-level defaults.
func (c *Client) SubscribeCaptures(config CaptureSubscriptionConfig, timeout time.Duration) error {
	h := c.begin()
	if h == nil {
		return errClosed
	}
	defer c.end()

	cConfig := C.ITSCAM_CaptureSubscriptionConfig{}
	if config.IncludeTrigger {
		cConfig.includeTrigger = 1
	}
	if config.IncludeSnapshot {
		cConfig.includeSnapshot = 1
	}
	if config.IncludeMetadata {
		cConfig.includeMetadata = 1
	}
	if config.EmbedComments {
		cConfig.embedComments = 1
	}
	if config.EmbedExif {
		cConfig.embedExif = 1
	}
	if config.EmbedSignature {
		cConfig.embedSignature = 1
	}
	cConfig.triggerQuality = C.int(config.TriggerQuality)
	cConfig.snapshotQuality = C.int(config.SnapshotQuality)

	result := C.ITSCAM_Client_subscribeCaptures(h, &cConfig, C.uint32_t(timeout.Milliseconds()))
	return errorFromCode(int(result))
}

// GetActiveProfile returns the active profile ID.
func (c *Client) GetActiveProfile(timeout time.Duration) (uint32, error) {
	h := c.begin()
	if h == nil {
		return 0, errClosed
	}
	defer c.end()

	var profileId C.uint32_t
	result := C.ITSCAM_Client_getActiveProfileId(h, C.uint32_t(timeout.Milliseconds()), &profileId)
	if err := errorFromCode(int(result)); err != nil {
		return 0, err
	}
	return uint32(profileId), nil
}

// SetActiveProfile sets the active profile.
func (c *Client) SetActiveProfile(profileId uint32, timeout time.Duration) error {
	h := c.begin()
	if h == nil {
		return errClosed
	}
	defer c.end()

	result := C.ITSCAM_Client_setActiveProfile(h, C.uint32_t(profileId), C.uint32_t(timeout.Milliseconds()))
	return errorFromCode(int(result))
}

// RequestSnapshot requests a snapshot capture from the camera.
// Prefer CaptureSnapshot when you need the returned frames directly.
func (c *Client) RequestSnapshot(timeout time.Duration) error {
	_, err := c.CaptureSnapshot(timeout)
	return err
}

// CaptureSnapshot requests a snapshot and returns all resulting frames.
func (c *Client) CaptureSnapshot(timeout time.Duration) ([]CaptureResult, error) {
	h := c.begin()
	if h == nil {
		return nil, errClosed
	}
	defer c.end()

	var resultArray *C.ITSCAM_CaptureResultArray
	result := C.ITSCAM_Client_captureSnapshot(h, nil, C.uint32_t(timeout.Milliseconds()), &resultArray)
	if err := errorFromCode(int(result)); err != nil {
		return nil, err
	}

	return takeCaptureResults(resultArray), nil
}

// GetLastFrame fetches the last preview frame as JPEG bytes.
func (c *Client) GetLastFrame(quality int, timeout time.Duration) ([]byte, error) {
	h := c.begin()
	if h == nil {
		return nil, errClosed
	}
	defer c.end()

	var outJpeg *C.ITSCAM_ByteArray
	result := C.ITSCAM_Client_getLastFrame(
		h,
		C.int(quality),
		C.uint32_t(timeout.Milliseconds()),
		&outJpeg)
	if err := errorFromCode(int(result)); err != nil {
		return nil, err
	}

	return takeByteArray(outJpeg), nil
}

// ListProfiles returns all camera profiles.
func (c *Client) ListProfiles(timeout time.Duration) ([]ProfileInfo, error) {
	h := c.begin()
	if h == nil {
		return nil, errClosed
	}
	defer c.end()

	var outProfiles *C.ITSCAM_ProfileArray
	result := C.ITSCAM_Client_listProfiles(
		h,
		C.uint32_t(timeout.Milliseconds()),
		&outProfiles)
	if err := errorFromCode(int(result)); err != nil {
		return nil, err
	}

	return takeProfiles(outProfiles), nil
}

// Reboot reboots the camera.
func (c *Client) Reboot(timeout time.Duration) error {
	h := c.begin()
	if h == nil {
		return errClosed
	}
	defer c.end()

	result := C.ITSCAM_Client_reboot(h, C.uint32_t(timeout.Milliseconds()))
	return errorFromCode(int(result))
}

// Address returns the camera address this client is connected to.
func (c *Client) Address() string {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.address
}

// Port returns the port this client is connected to.
func (c *Client) Port() uint16 {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.port
}

// Helper to convert null-terminated C string in byte slice to Go string
func cStringToGo(b []byte) string {
	for i, v := range b {
		if v == 0 {
			return string(b[:i])
		}
	}
	return string(b)
}

// errorFromCode converts an error code to a Go error.
func errorFromCode(code int) error {
	switch code {
	case 0:
		return nil
	case 1:
		return &ConnectionError{msg: "connection failed"}
	case 2:
		return &TimeoutError{msg: "operation timed out"}
	case 3:
		return &AuthError{msg: "authentication failed"}
	case 4:
		return errors.New("invalid parameter")
	case 5:
		return errors.New("server error")
	case 6:
		return &ConnectionError{msg: "disconnected"}
	case 7:
		return errors.New("unknown error")
	case 8:
		return errors.New("null handle")
	case 9:
		return errors.New("allocation failed")
	default:
		return fmt.Errorf("error code: %d", code)
	}
}

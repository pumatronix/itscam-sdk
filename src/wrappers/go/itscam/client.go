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
//	if err := client.Connect("192.168.1.100", 50000, 5*time.Second); err != nil {
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
type Client struct {
	handle  *C.ITSCAM_Client
	mu      sync.Mutex
	closed  bool

	// Connection info (for reconnection/info)
	address string
	port    uint16

	// Callback handlers
	captureCallback         func(*CaptureResult)
	snapshotCallback        func(*CaptureResult)
	disconnectCallback      func(string)
	connectionStateCallback func(ConnectionState, string)
	logCallback             func(LogLevel, string)
}

// NewClient creates a new ITSCAM client.
func NewClient() (*Client, error) {
	handle := C.ITSCAM_Client_create()
	if handle == nil {
		return nil, errors.New("failed to create client")
	}

	client := &Client{handle: handle}
	runtime.SetFinalizer(client, (*Client).Close)
	return client, nil
}

// Close releases the client resources.
func (c *Client) Close() error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return nil
	}

	unregisterClient(c)
	C.ITSCAM_Client_destroy(c.handle)
	c.handle = nil
	c.closed = true
	runtime.SetFinalizer(c, nil)
	return nil
}

// Connect establishes a connection to the camera.
func (c *Client) Connect(address string, port uint16, timeout time.Duration) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return errors.New("client is closed")
	}

	caddr := C.CString(address)
	defer C.free(unsafe.Pointer(caddr))

	c.address = address
	c.port = port

	result := C.ITSCAM_Client_connect(
		c.handle,
		caddr,
		C.uint16_t(port),
		C.uint32_t(timeout.Milliseconds()),
		nil) // no auto-reconnect config
	return errorFromCode(int(result))
}

// ConnectWithReconnect establishes a connection with auto-reconnect enabled.
func (c *Client) ConnectWithReconnect(address string, port uint16, timeout time.Duration, reconnect AutoReconnectConfig) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return errors.New("client is closed")
	}

	caddr := C.CString(address)
	defer C.free(unsafe.Pointer(caddr))

	c.address = address
	c.port = port

	var cReconnect C.ITSCAM_AutoReconnectConfig
	if reconnect.Enabled {
		cReconnect.enabled = 1
	} else {
		cReconnect.enabled = 0
	}
	cReconnect.intervalMs = C.uint32_t(reconnect.Interval.Milliseconds())
	cReconnect.maxRetries = C.uint32_t(reconnect.MaxAttempts)

	result := C.ITSCAM_Client_connect(
		c.handle,
		caddr,
		C.uint16_t(port),
		C.uint32_t(timeout.Milliseconds()),
		&cReconnect)
	return errorFromCode(int(result))
}

// Disconnect closes the connection to the camera.
func (c *Client) Disconnect() {
	c.mu.Lock()
	defer c.mu.Unlock()

	if !c.closed {
		C.ITSCAM_Client_disconnect(c.handle)
	}
}

// IsConnected returns whether the client is connected.
func (c *Client) IsConnected() bool {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return false
	}
	return C.ITSCAM_Client_isConnected(c.handle) != 0
}

// Authenticate authenticates with the camera.
func (c *Client) Authenticate(password string, timeout time.Duration) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return errors.New("client is closed")
	}

	cpass := C.CString(password)
	defer C.free(unsafe.Pointer(cpass))

	result := C.ITSCAM_Client_authenticate(c.handle, cpass, C.uint32_t(timeout.Milliseconds()))
	return errorFromCode(int(result))
}

// Subscribe subscribes to camera events.
func (c *Client) Subscribe(events EventSubscription, timeout time.Duration) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return errors.New("client is closed")
	}

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

	result := C.ITSCAM_Client_subscribe(c.handle, &cEvents, C.uint32_t(timeout.Milliseconds()))
	return errorFromCode(int(result))
}

// SubscribeCaptures subscribes to capture events using the SDK high-level defaults.
func (c *Client) SubscribeCaptures(config CaptureSubscriptionConfig, timeout time.Duration) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return errors.New("client is closed")
	}

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

	result := C.ITSCAM_Client_subscribeCaptures(c.handle, &cConfig, C.uint32_t(timeout.Milliseconds()))
	return errorFromCode(int(result))
}

// GetActiveProfile returns the active profile ID.
func (c *Client) GetActiveProfile(timeout time.Duration) (uint32, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return 0, errors.New("client is closed")
	}

	var profileId C.uint32_t
	result := C.ITSCAM_Client_getActiveProfileId(c.handle, C.uint32_t(timeout.Milliseconds()), &profileId)
	if err := errorFromCode(int(result)); err != nil {
		return 0, err
	}
	return uint32(profileId), nil
}

// SetActiveProfile sets the active profile.
func (c *Client) SetActiveProfile(profileId uint32, timeout time.Duration) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return errors.New("client is closed")
	}

	result := C.ITSCAM_Client_setActiveProfile(c.handle, C.uint32_t(profileId), C.uint32_t(timeout.Milliseconds()))
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
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return nil, errors.New("client is closed")
	}

	var resultArray *C.ITSCAM_CaptureResultArray
	result := C.ITSCAM_Client_captureSnapshot(c.handle, nil, C.uint32_t(timeout.Milliseconds()), &resultArray)
	if err := errorFromCode(int(result)); err != nil {
		return nil, err
	}

	return takeCaptureResults(resultArray), nil
}

// GetLastFrame fetches the last preview frame as JPEG bytes.
func (c *Client) GetLastFrame(quality int, timeout time.Duration) ([]byte, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return nil, errors.New("client is closed")
	}

	var outJpeg *C.ITSCAM_ByteArray
	result := C.ITSCAM_Client_getLastFrame(
		c.handle,
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
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return nil, errors.New("client is closed")
	}

	var outProfiles *C.ITSCAM_ProfileArray
	result := C.ITSCAM_Client_listProfiles(
		c.handle,
		C.uint32_t(timeout.Milliseconds()),
		&outProfiles)
	if err := errorFromCode(int(result)); err != nil {
		return nil, err
	}

	return takeProfiles(outProfiles), nil
}

// Reboot reboots the camera.
func (c *Client) Reboot(timeout time.Duration) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if c.closed {
		return errors.New("client is closed")
	}

	result := C.ITSCAM_Client_reboot(c.handle, C.uint32_t(timeout.Milliseconds()))
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

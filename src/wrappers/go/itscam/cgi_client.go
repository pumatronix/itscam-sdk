// Package itscam - CGI client (Go wrapper).
//
// CgiClient covers the legacy ITSCAM CGI endpoints:
//
//	GET /api/snapshot.cgi    -- on-demand capture (multi-exposure aware)
//	GET /api/lastframe.cgi   -- most recent preview frame
//	GET /api/mjpegvideo.cgi  -- continuous MJPEG stream
//	GET /api/reboot.cgi      -- restart camera-daemon
//
// HTTPS is supported through the statically-linked mbedTLS backend.

package itscam

/*
#cgo !static LDFLAGS: -litscam_sdk
#cgo static,linux LDFLAGS: ${SRCDIR}/../../../core/build/linux/libitscam_sdk.a -lstdc++ -lpthread -lm
#cgo static,windows CFLAGS: -DITSCAM_SDK_STATIC
#cgo static,windows LDFLAGS: ${SRCDIR}/../../../core/build/windows/libitscam_sdk_static.a -lws2_32 -lstdc++ -lm -static
#include <stdlib.h>
#include "../../../core/c_api/itscam_cgi_client_c.h"

// Forward declaration for the Go callback (without const to match cgo export).
extern void itscamCgiGoCallback(ITSCAM_CgiStreamFrame* frame, void* userData);
*/
import "C"
import (
	"errors"
	"runtime"
	"runtime/cgo"
	"sync"
	"unsafe"
)

// SnapshotCgiRequest mirrors the C struct ITSCAM_CgiSnapshotRequest.
type SnapshotCgiRequest struct {
	Shutters     []int32
	Gains        []int32
	Quality      int32
	Mosaic       bool
	Format       string
	Scenario     int32
	Crop         string
	TextOverlay  string
	UserMetadata map[string]string
}

// NewSnapshotCgiRequest returns a request with defaults that match the
// camera's "just take a snapshot with current settings" behaviour.
func NewSnapshotCgiRequest() *SnapshotCgiRequest {
	return &SnapshotCgiRequest{Quality: -1, Scenario: -1}
}

// CgiImage is a single image returned by lastframe.cgi or snapshot.cgi.
type CgiImage struct {
	MimeType string
	Data     []byte
}

// CgiStreamFrame is one frame delivered through StartMjpegStream.
type CgiStreamFrame struct {
	Sequence uint64
	MimeType string
	Data     []byte
}

// StreamCallback is invoked from the SDK's native worker thread.
type StreamCallback func(CgiStreamFrame)

// CgiClient wraps ItscamCgiClient.
type CgiClient struct {
	handle *C.ITSCAM_CgiClient
	mu     sync.Mutex
	closed bool

	streamCb     StreamCallback
	streamHandle cgo.Handle
}

// NewCgiClient creates a new CGI client.
func NewCgiClient() (*CgiClient, error) {
	h := C.ITSCAM_CgiClient_create()
	if h == nil {
		return nil, errors.New("failed to allocate ITSCAM CGI client")
	}
	c := &CgiClient{handle: h}
	runtime.SetFinalizer(c, (*CgiClient).Close)
	return c, nil
}

// Close releases the client.  Safe to call multiple times.
func (c *CgiClient) Close() error {
	c.mu.Lock(); defer c.mu.Unlock()
	if c.closed {
		return nil
	}
	c.closed = true
	if c.handle != nil {
		C.ITSCAM_CgiClient_stopMjpegStream(c.handle)
		C.ITSCAM_CgiClient_destroy(c.handle)
		c.handle = nil
	}
	if c.streamHandle != 0 {
		c.streamHandle.Delete()
		c.streamHandle = 0
	}
	runtime.SetFinalizer(c, nil)
	return nil
}

// ============================================================================
//  Configuration
// ============================================================================

func (c *CgiClient) SetBaseUrl(host string, port uint16, scheme string) error {
	c.mu.Lock(); defer c.mu.Unlock()
	cHost := C.CString(host)
	defer C.free(unsafe.Pointer(cHost))
	cScheme := C.CString(scheme)
	defer C.free(unsafe.Pointer(cScheme))
	return errorFromCode(int(C.ITSCAM_CgiClient_setBaseUrl(
		c.handle, cHost, C.uint16_t(port), cScheme)))
}

func (c *CgiClient) SetApiPrefix(prefix string) {
	c.mu.Lock(); defer c.mu.Unlock()
	cp := C.CString(prefix)
	defer C.free(unsafe.Pointer(cp))
	C.ITSCAM_CgiClient_setApiPrefix(c.handle, cp)
}

func (c *CgiClient) SetCaCertFile(path string) {
	c.mu.Lock(); defer c.mu.Unlock()
	cp := C.CString(path)
	defer C.free(unsafe.Pointer(cp))
	C.ITSCAM_CgiClient_setCaCertFile(c.handle, cp)
}

func (c *CgiClient) SetCaCertData(pem string) {
	c.mu.Lock(); defer c.mu.Unlock()
	cp := C.CString(pem)
	defer C.free(unsafe.Pointer(cp))
	C.ITSCAM_CgiClient_setCaCertData(c.handle, cp)
}

func (c *CgiClient) SetVerifyServerCertificate(verify bool) {
	c.mu.Lock(); defer c.mu.Unlock()
	v := C.int(0)
	if verify {
		v = 1
	}
	C.ITSCAM_CgiClient_setVerifyServerCertificate(c.handle, v)
}

func (c *CgiClient) SetClientCertificate(certPem, keyPem string) {
	c.mu.Lock(); defer c.mu.Unlock()
	cc := C.CString(certPem)
	defer C.free(unsafe.Pointer(cc))
	ck := C.CString(keyPem)
	defer C.free(unsafe.Pointer(ck))
	C.ITSCAM_CgiClient_setClientCertificate(c.handle, cc, ck)
}

// ============================================================================
//  Authentication
// ============================================================================

func (c *CgiClient) Login(user, pass string, timeoutMs uint32) error {
	c.mu.Lock(); defer c.mu.Unlock()
	cu := C.CString(user)
	defer C.free(unsafe.Pointer(cu))
	cp := C.CString(pass)
	defer C.free(unsafe.Pointer(cp))
	return errorFromCode(int(C.ITSCAM_CgiClient_login(
		c.handle, cu, cp, C.uint32_t(timeoutMs))))
}

func (c *CgiClient) SetAuthToken(token string) {
	c.mu.Lock(); defer c.mu.Unlock()
	ct := C.CString(token)
	defer C.free(unsafe.Pointer(ct))
	C.ITSCAM_CgiClient_setAuthToken(c.handle, ct)
}

func (c *CgiClient) ClearAuthToken() {
	c.mu.Lock(); defer c.mu.Unlock()
	C.ITSCAM_CgiClient_clearAuthToken(c.handle)
}

// ============================================================================
//  /api/lastframe.cgi
// ============================================================================

// GetLastFrame fetches the most recent preview image.
func (c *CgiClient) GetLastFrame(timeoutMs uint32) (*CgiImage, error) {
	c.mu.Lock(); defer c.mu.Unlock()
	var out *C.ITSCAM_CgiImage
	rc := C.ITSCAM_CgiClient_getLastFrame(c.handle,
		C.uint32_t(timeoutMs), &out)
	if err := errorFromCode(int(rc)); err != nil {
		if out != nil {
			C.ITSCAM_CgiImage_destroy(out)
		}
		return nil, err
	}
	defer C.ITSCAM_CgiImage_destroy(out)
	return consumeImage(out), nil
}

// ============================================================================
//  /api/snapshot.cgi
// ============================================================================

// GetSnapshot triggers a capture and returns one or more images
// (multi-exposure groups yield multiple entries unless Mosaic is set).
func (c *CgiClient) GetSnapshot(req *SnapshotCgiRequest, timeoutMs uint32) ([]CgiImage, error) {
	c.mu.Lock(); defer c.mu.Unlock()
	if req == nil {
		req = NewSnapshotCgiRequest()
	}

	var native C.ITSCAM_CgiSnapshotRequest
	native.quality = C.int32_t(req.Quality)
	if req.Mosaic {
		native.mosaic = 1
	}
	native.scenario = C.int32_t(req.Scenario)

	cFormat := cStringOrNil(req.Format)
	defer freeIfNotNil(cFormat)
	native.format = cFormat

	cCrop := cStringOrNil(req.Crop)
	defer freeIfNotNil(cCrop)
	native.crop = cCrop

	cText := cStringOrNil(req.TextOverlay)
	defer freeIfNotNil(cText)
	native.textOverlay = cText

	var shuttersBuf unsafe.Pointer
	if len(req.Shutters) > 0 {
		shuttersBuf = C.malloc(C.size_t(len(req.Shutters)) *
			C.sizeof_int32_t)
		defer C.free(shuttersBuf)
		dst := unsafe.Slice((*C.int32_t)(shuttersBuf), len(req.Shutters))
		for i, v := range req.Shutters {
			dst[i] = C.int32_t(v)
		}
		native.shutters = (*C.int32_t)(shuttersBuf)
		native.shuttersLen = C.size_t(len(req.Shutters))
	}

	var gainsBuf unsafe.Pointer
	if len(req.Gains) > 0 {
		gainsBuf = C.malloc(C.size_t(len(req.Gains)) *
			C.sizeof_int32_t)
		defer C.free(gainsBuf)
		dst := unsafe.Slice((*C.int32_t)(gainsBuf), len(req.Gains))
		for i, v := range req.Gains {
			dst[i] = C.int32_t(v)
		}
		native.gains = (*C.int32_t)(gainsBuf)
		native.gainsLen = C.size_t(len(req.Gains))
	}

	var keysSlice, valsSlice []*C.char
	if len(req.UserMetadata) > 0 {
		for k, v := range req.UserMetadata {
			ck := C.CString(k)
			cv := C.CString(v)
			defer C.free(unsafe.Pointer(ck))
			defer C.free(unsafe.Pointer(cv))
			keysSlice = append(keysSlice, ck)
			valsSlice = append(valsSlice, cv)
		}
		keysSlice = append(keysSlice, nil)
		valsSlice = append(valsSlice, nil)
		native.userMetadataKeys   = (**C.char)(unsafe.Pointer(&keysSlice[0]))
		native.userMetadataValues = (**C.char)(unsafe.Pointer(&valsSlice[0]))
	}

	var outArr *C.ITSCAM_CgiImageArray
	rc := C.ITSCAM_CgiClient_getSnapshot(c.handle, &native,
		C.uint32_t(timeoutMs), &outArr)
	if err := errorFromCode(int(rc)); err != nil {
		if outArr != nil {
			C.ITSCAM_CgiImageArray_destroy(outArr)
		}
		return nil, err
	}
	defer C.ITSCAM_CgiImageArray_destroy(outArr)

	n := int(C.ITSCAM_CgiImageArray_size(outArr))
	out := make([]CgiImage, n)
	for i := 0; i < n; i++ {
		entry := C.ITSCAM_CgiImageArray_get(outArr, C.size_t(i))
		if entry == nil {
			continue
		}
		out[i] = *borrowImage(entry)
	}
	return out, nil
}

// ============================================================================
//  /api/mjpegvideo.cgi
// ============================================================================

// StartMjpegStream begins streaming MJPEG frames.  Subsequent frames are
// delivered to cb on the SDK's worker thread.  Use a channel or sync
// primitive if you need to consume them from a different goroutine.
func (c *CgiClient) StartMjpegStream(cb StreamCallback, timeoutMs uint32) error {
	c.mu.Lock(); defer c.mu.Unlock()
	if cb == nil {
		return errors.New("callback must not be nil")
	}
	if c.streamHandle != 0 {
		return errors.New("mjpeg stream already running")
	}
	c.streamCb = cb
	c.streamHandle = cgo.NewHandle(c)
	rc := C.ITSCAM_CgiClient_startMjpegStream(c.handle,
		(C.ITSCAM_CgiStreamCallback)(C.itscamCgiGoCallback),
		unsafe.Pointer(uintptr(c.streamHandle)),
		C.uint32_t(timeoutMs))
	if err := errorFromCode(int(rc)); err != nil {
		c.streamHandle.Delete()
		c.streamHandle = 0
		c.streamCb = nil
		return err
	}
	return nil
}

// StopMjpegStream cancels an active stream.  Safe to call when no
// stream is running.
func (c *CgiClient) StopMjpegStream() {
	c.mu.Lock(); defer c.mu.Unlock()
	if c.handle == nil {
		return
	}
	C.ITSCAM_CgiClient_stopMjpegStream(c.handle)
	if c.streamHandle != 0 {
		c.streamHandle.Delete()
		c.streamHandle = 0
	}
	c.streamCb = nil
}

// IsMjpegStreamRunning reports whether a stream is active.
func (c *CgiClient) IsMjpegStreamRunning() bool {
	c.mu.Lock(); defer c.mu.Unlock()
	return C.ITSCAM_CgiClient_isMjpegStreamRunning(c.handle) != 0
}

// ============================================================================
//  /api/trigger.cgi force / /api/reboot.cgi
// ============================================================================

func (c *CgiClient) ForceTrigger(timeoutMs uint32) (string, error) {
	c.mu.Lock(); defer c.mu.Unlock()
	var out *C.ITSCAM_String
	rc := C.ITSCAM_CgiClient_forceTrigger(c.handle,
		C.uint32_t(timeoutMs), &out)
	body := takeString(out)
	return body, errorFromCode(int(rc))
}

func (c *CgiClient) Reboot(timeoutMs uint32) (string, error) {
	c.mu.Lock(); defer c.mu.Unlock()
	var out *C.ITSCAM_String
	rc := C.ITSCAM_CgiClient_reboot(c.handle,
		C.uint32_t(timeoutMs), &out)
	body := takeString(out)
	return body, errorFromCode(int(rc))
}

// ============================================================================
//  Helpers
// ============================================================================

func cStringOrNil(s string) *C.char {
	if s == "" {
		return nil
	}
	return C.CString(s)
}

func freeIfNotNil(p *C.char) {
	if p != nil {
		C.free(unsafe.Pointer(p))
	}
}

func consumeImage(h *C.ITSCAM_CgiImage) *CgiImage {
	if h == nil {
		return &CgiImage{}
	}
	mime := C.GoString(C.ITSCAM_CgiImage_mimeType(h))
	size := int(C.ITSCAM_CgiImage_size(h))
	ptr  := C.ITSCAM_CgiImage_data(h)
	var data []byte
	if ptr != nil && size > 0 {
		data = C.GoBytes(unsafe.Pointer(ptr), C.int(size))
	}
	return &CgiImage{MimeType: mime, Data: data}
}

func borrowImage(h *C.ITSCAM_CgiImage) *CgiImage {
	return consumeImage(h) // no ownership transfer; only reads via accessors
}

// itscamCgiGoCallback is the trampoline registered with the native CGI
// stream.  Exported so the C code in the preamble can call it.
//
//export itscamCgiGoCallback
func itscamCgiGoCallback(frame *C.ITSCAM_CgiStreamFrame, userData unsafe.Pointer) {
	if frame == nil || userData == nil {
		return
	}
	h := cgo.Handle(uintptr(userData))
	client, ok := h.Value().(*CgiClient)
	if !ok || client.streamCb == nil {
		return
	}
	mime := ""
	if frame.mimeType != nil {
		mime = C.GoString(frame.mimeType)
	}
	var data []byte
	if frame.data != nil && frame.dataLen > 0 {
		data = C.GoBytes(unsafe.Pointer(frame.data),
			C.int(frame.dataLen))
	}
	defer func() {
		// Never let a panic cross back into native code.
		_ = recover()
	}()
	client.streamCb(CgiStreamFrame{
		Sequence: uint64(frame.sequence),
		MimeType: mime,
		Data:     data,
	})
}

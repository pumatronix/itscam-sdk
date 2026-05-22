package itscam

import "time"

// Timestamp represents a timestamp with millisecond precision.
type Timestamp struct {
	Year           int
	Month          int
	Day            int
	Hour           int
	Minute         int
	Second         int
	Millisecond    int
	TimezoneOffset int // timezone offset in minutes
}

// ToTime converts to a time.Time object.
func (ts Timestamp) ToTime() time.Time {
	return time.Date(ts.Year, time.Month(ts.Month), ts.Day,
		ts.Hour, ts.Minute, ts.Second, ts.Millisecond*1e6, time.Local)
}

// ToISO8601 returns ISO 8601 formatted string.
func (ts Timestamp) ToISO8601() string {
	return ts.ToTime().Format("2006-01-02T15:04:05.000")
}

func (ts Timestamp) String() string {
	return ts.ToISO8601()
}

// FrameInfo contains metadata about a captured frame.
// Maps to ITSCAM_FrameInfo from C API.
type FrameInfo struct {
	RequestID      uint64
	FrameCount     uint64
	MultiExpIndex  int
	MultiExpLength int
	Shutter        int
	Gain           float32
	Width          int
	Height         int
	Timestamp      Timestamp
}

// CaptureResult represents a capture result with image data.
type CaptureResult struct {
	FrameInfo FrameInfo
	JpegData  []byte
	Plates    []string
}

// ProfileInfo contains profile information.
type ProfileInfo struct {
	Index       int
	Name        string
	Description string
}

// ConnectionState represents the connection state.
// Values must match the C++ SDK enum in itscam_types.h
type ConnectionState int

const (
	StateConnected    ConnectionState = iota // 0 - Connected
	StateDisconnected                        // 1 - Disconnected
	StateReconnecting                        // 2 - Reconnecting
	StateReconnected                         // 3 - Reconnected
)

func (s ConnectionState) String() string {
	switch s {
	case StateConnected:
		return "Connected"
	case StateDisconnected:
		return "Disconnected"
	case StateReconnecting:
		return "Reconnecting"
	case StateReconnected:
		return "Reconnected"
	default:
		return "Unknown"
	}
}

// LogLevel represents the logging level.
// Values must match the C++ SDK enum in itscam_types.h.
type LogLevel int

const (
	LogInfo  LogLevel = 0 // Info level
	LogError LogLevel = 1 // Error level
)

func (l LogLevel) String() string {
	switch l {
	case LogInfo:
		return "INFO"
	case LogError:
		return "ERROR"
	default:
		return "UNKNOWN"
	}
}

// AutoReconnectConfig configures auto-reconnect behavior.
type AutoReconnectConfig struct {
	Enabled     bool
	Interval    time.Duration
	MaxAttempts int
}

// DefaultAutoReconnectConfig returns the default auto-reconnect configuration.
func DefaultAutoReconnectConfig() AutoReconnectConfig {
	return AutoReconnectConfig{
		Enabled:     true,
		Interval:    5 * time.Second,
		MaxAttempts: 0, // unlimited
	}
}

// EventSubscription configures which events to receive.
type EventSubscription struct {
	Pipeline         bool
	TriggerMetadata  bool
	TriggerImage     bool
	SnapshotMetadata bool
	SnapshotImage    bool
	PreviewMetadata  bool
	PreviewImage     bool
	GPIO             bool
	Serial1          bool
	Serial2          bool
}

// CaptureSubscriptionConfig configures the high-level capture subscription API.
type CaptureSubscriptionConfig struct {
	IncludeTrigger  bool
	IncludeSnapshot bool
	IncludeMetadata bool
	EmbedComments   bool
	EmbedExif       bool
	EmbedSignature  bool
	TriggerQuality  int
	SnapshotQuality int
}

// DefaultCaptureSubscriptionConfig returns the SDK common-case defaults.
func DefaultCaptureSubscriptionConfig() CaptureSubscriptionConfig {
	return CaptureSubscriptionConfig{
		IncludeTrigger:  true,
		IncludeSnapshot: true,
		IncludeMetadata: true,
		EmbedComments:   true,
		EmbedExif:       true,
		EmbedSignature:  false,
		TriggerQuality:  -1,
		SnapshotQuality: -1,
	}
}

// AllTriggers returns a subscription for all trigger events.
func AllTriggers() EventSubscription {
	return EventSubscription{
		TriggerMetadata: true,
		TriggerImage:    true,
	}
}

// AllSnapshots returns a subscription for all snapshot events.
func AllSnapshots() EventSubscription {
	return EventSubscription{
		SnapshotMetadata: true,
		SnapshotImage:    true,
	}
}

// AllEvents returns a subscription for all events.
func AllEvents() EventSubscription {
	return EventSubscription{
		Pipeline:         true,
		TriggerMetadata:  true,
		TriggerImage:     true,
		SnapshotMetadata: true,
		SnapshotImage:    true,
		PreviewMetadata:  true,
		PreviewImage:     true,
		GPIO:             true,
		Serial1:          true,
		Serial2:          true,
	}
}

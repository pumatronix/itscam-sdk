/*
 * app.go
 *
 * ITSCAM SDK Go GUI Example - Backend Application Logic
 *
 * This file contains the Go backend that binds to the Wails frontend.
 * It provides methods for connecting to cameras, receiving captures,
 * and querying device information.
 *
 * Copyright (c) 2026 Pumatronix
 */

package main

import (
	"context"
	"encoding/base64"
	"fmt"
	"os"
	"path/filepath"
	"sync"
	"time"

	"github.com/pumatronix/itscam-sdk-go/itscam"
	"github.com/wailsapp/wails/v2/pkg/runtime"
)

// FrameSource represents the source of captured frames
type FrameSource string

const (
	FrameSourceTrigger  FrameSource = "trigger"
	FrameSourceSnapshot FrameSource = "snapshot"
)

// DeviceInfo contains device information for the frontend
type DeviceInfo struct {
	ActiveProfile uint32 `json:"activeProfile"`
	Address       string `json:"address"`
	Port          uint16 `json:"port"`
}

// CaptureEvent represents a capture event sent to the frontend
type CaptureEvent struct {
	Source     string  `json:"source"`
	RequestID  uint64  `json:"requestId"`
	FrameCount uint64  `json:"frameCount"`
	Width      int     `json:"width"`
	Height     int     `json:"height"`
	Timestamp  string  `json:"timestamp"`
	Plates     []string `json:"plates"`
	ImageB64   string  `json:"imageB64"`
	ImageSize  int     `json:"imageSize"`
	// Extended metadata
	MultiExpIndex  int     `json:"multiExpIndex"`
	MultiExpLength int     `json:"multiExpLength"`
	Shutter        int     `json:"shutter"`
	Gain           float32 `json:"gain"`
}

// ConnectionStatus represents the connection status
type ConnectionStatus struct {
	Connected bool   `json:"connected"`
	State     string `json:"state"`
	Message   string `json:"message"`
}

// LogEntry represents a log entry for the frontend
type LogEntry struct {
	Level     string `json:"level"`
	Message   string `json:"message"`
	Timestamp string `json:"timestamp"`
}

// SubscriptionConfig represents the subscription configuration
type SubscriptionConfig struct {
	Trigger  bool `json:"trigger"`
	Snapshot bool `json:"snapshot"`
}

// SaveResult represents the result of a save operation
type SaveResult struct {
	Success  bool   `json:"success"`
	FilePath string `json:"filePath"`
	Error    string `json:"error,omitempty"`
}

// App struct holds the application state
type App struct {
	ctx    context.Context
	client *itscam.Client
	mu     sync.Mutex

	connected    bool
	cameraIP     string
	cameraPort   uint16
	captureCount int64

	// Subscription state
	subscriptionConfig SubscriptionConfig

	// Save settings
	saveDirectory  string
	autoSave       bool
}

// NewApp creates a new App instance
func NewApp() *App {
	return &App{
		cameraPort:    60000,
		saveDirectory: "",
		autoSave:      false,
	}
}

// startup is called when the app starts
func (a *App) startup(ctx context.Context) {
	a.ctx = ctx
}

// shutdown is called when the app closes
func (a *App) shutdown(ctx context.Context) {
	a.mu.Lock()
	client := a.client
	a.client = nil
	a.mu.Unlock()

	if client != nil {
		client.Close()
	}
}

// Connect connects to the camera
func (a *App) Connect(ip string, port int, password string) error {
	// Get and clear old client under lock
	a.mu.Lock()
	oldClient := a.client
	a.client = nil
	a.connected = false
	a.mu.Unlock()

	// Close old client outside of lock
	if oldClient != nil {
		oldClient.Close()
	}

	// Create new client
	client, err := itscam.NewClient()
	if err != nil {
		a.emitLog("ERROR", fmt.Sprintf("Failed to create client: %v", err))
		return fmt.Errorf("failed to create client: %w", err)
	}

	// Set up callbacks (these don't block)
	client.SetCaptureCallback(a.onTriggerCapture)
	client.SetSnapshotCallback(a.onSnapshotCapture)
	client.SetDisconnectCallback(a.onDisconnect)
	client.SetConnectionStateCallback(a.onConnectionState)
	client.SetLogCallback(a.onLog)

	// Connect with timeout and auto-reconnect (outside of lock to allow callbacks)
	a.emitLog("INFO", fmt.Sprintf("Connecting to %s:%d...", ip, port))
	reconnectConfig := itscam.AutoReconnectConfig{
		Enabled:  true,
		Interval: 5 * time.Second,
	}
	if err := client.ConnectWithReconnect(ip, uint16(port), 10*time.Second, reconnectConfig); err != nil {
		client.Close()
		a.emitLog("ERROR", fmt.Sprintf("Connection failed: %v", err))
		return fmt.Errorf("connection failed: %w", err)
	}

	// Authenticate only if password is provided (outside of lock)
	if password != "" {
		a.emitLog("INFO", "Connected, authenticating...")
		if err := client.Authenticate(password, 10*time.Second); err != nil {
			client.Close()
			a.emitLog("ERROR", fmt.Sprintf("Authentication failed: %v", err))
			return fmt.Errorf("authentication failed: %w", err)
		}
		a.emitLog("INFO", "Connected and authenticated successfully")
	} else {
		a.emitLog("INFO", "Connected (no authentication)")
	}

	// Update state under lock
	a.mu.Lock()
	a.client = client
	a.connected = true
	a.cameraIP = ip
	a.cameraPort = uint16(port)
	a.mu.Unlock()

	a.emitConnectionStatus(true, "Connected", "")

	return nil
}

// Disconnect disconnects from the camera
func (a *App) Disconnect() {
	a.mu.Lock()
	client := a.client
	a.client = nil
	a.connected = false
	a.mu.Unlock()

	if client != nil {
		client.Close()
		a.emitLog("INFO", "Disconnected")
		a.emitConnectionStatus(false, "Disconnected", "User initiated disconnect")
	}
}

// Subscribe starts subscribing to capture events
func (a *App) Subscribe() error {
	a.mu.Lock()
	client := a.client
	a.mu.Unlock()

	if client == nil {
		return fmt.Errorf("not connected")
	}

	// Subscribe to trigger events by default
	config := itscam.DefaultCaptureSubscriptionConfig()
	config.IncludeSnapshot = false
	if err := client.SubscribeCaptures(config, 10*time.Second); err != nil {
		a.emitLog("ERROR", fmt.Sprintf("Subscribe failed: %v", err))
		return fmt.Errorf("subscribe failed: %w", err)
	}

	a.mu.Lock()
	a.subscriptionConfig = SubscriptionConfig{Trigger: true, Snapshot: false}
	a.mu.Unlock()

	a.emitLog("INFO", "Subscribed to trigger events")
	return nil
}

// SubscribeWithConfig subscribes to events based on the provided configuration
func (a *App) SubscribeWithConfig(config SubscriptionConfig) error {
	a.mu.Lock()
	client := a.client
	a.mu.Unlock()

	if client == nil {
		return fmt.Errorf("not connected")
	}

	captureConfig := itscam.DefaultCaptureSubscriptionConfig()
	captureConfig.IncludeTrigger = config.Trigger
	captureConfig.IncludeSnapshot = config.Snapshot

	if err := client.SubscribeCaptures(captureConfig, 10*time.Second); err != nil {
		a.emitLog("ERROR", fmt.Sprintf("Subscribe failed: %v", err))
		return fmt.Errorf("subscribe failed: %w", err)
	}

	a.mu.Lock()
	a.subscriptionConfig = config
	a.mu.Unlock()

	sources := []string{}
	if config.Trigger {
		sources = append(sources, "trigger")
	}
	if config.Snapshot {
		sources = append(sources, "snapshot")
	}
	a.emitLog("INFO", fmt.Sprintf("Subscribed to events: %v", sources))
	return nil
}

// GetSubscriptionConfig returns the current subscription configuration
func (a *App) GetSubscriptionConfig() SubscriptionConfig {
	a.mu.Lock()
	defer a.mu.Unlock()
	return a.subscriptionConfig
}

// Unsubscribe stops subscribing to capture events
// Note: The SDK doesn't have an explicit unsubscribe, so we just log this
func (a *App) Unsubscribe() error {
	a.mu.Lock()
	client := a.client
	a.mu.Unlock()

	if client == nil {
		return fmt.Errorf("not connected")
	}

	// Subscribe to nothing to effectively unsubscribe
	if err := client.Subscribe(itscam.EventSubscription{}, 10*time.Second); err != nil {
		return fmt.Errorf("unsubscribe failed: %w", err)
	}

	a.mu.Lock()
	a.subscriptionConfig = SubscriptionConfig{Trigger: false, Snapshot: false}
	a.mu.Unlock()

	a.emitLog("INFO", "Unsubscribed from capture events")
	return nil
}

// GetDeviceInfo returns device information
func (a *App) GetDeviceInfo() (*DeviceInfo, error) {
	a.mu.Lock()
	client := a.client
	a.mu.Unlock()

	if client == nil {
		return nil, fmt.Errorf("not connected")
	}

	activeProfile, err := client.GetActiveProfile(5 * time.Second)
	if err != nil {
		a.emitLog("WARNING", fmt.Sprintf("Failed to get active profile: %v", err))
		activeProfile = 0
	}

	return &DeviceInfo{
		ActiveProfile: activeProfile,
		Address:       client.Address(),
		Port:          client.Port(),
	}, nil
}

// GetConnectionStatus returns the current connection status
func (a *App) GetConnectionStatus() *ConnectionStatus {
	a.mu.Lock()
	defer a.mu.Unlock()

	state := "Disconnected"
	if a.connected {
		state = "Connected"
	}

	return &ConnectionStatus{
		Connected: a.connected,
		State:     state,
		Message:   "",
	}
}

// RequestSnapshot requests a snapshot from the camera
func (a *App) RequestSnapshot() error {
	a.mu.Lock()
	client := a.client
	a.mu.Unlock()

	if client == nil {
		return fmt.Errorf("not connected")
	}

	a.emitLog("INFO", "Requesting snapshot...")
	if err := client.RequestSnapshot(10 * time.Second); err != nil {
		a.emitLog("ERROR", fmt.Sprintf("Snapshot request failed: %v", err))
		return fmt.Errorf("snapshot request failed: %w", err)
	}

	return nil
}

// SetSaveDirectory sets the directory for saving images
func (a *App) SetSaveDirectory(dir string) error {
	if dir == "" {
		a.mu.Lock()
		a.saveDirectory = ""
		a.mu.Unlock()
		a.emitLog("INFO", "Save directory cleared")
		return nil
	}

	// Ensure directory exists
	if err := os.MkdirAll(dir, 0755); err != nil {
		return fmt.Errorf("failed to create directory: %w", err)
	}

	a.mu.Lock()
	a.saveDirectory = dir
	a.mu.Unlock()
	a.emitLog("INFO", fmt.Sprintf("Save directory set to: %s", dir))
	return nil
}

// GetSaveDirectory returns the current save directory
func (a *App) GetSaveDirectory() string {
	a.mu.Lock()
	defer a.mu.Unlock()
	return a.saveDirectory
}

// SetAutoSave enables or disables automatic saving of frames
func (a *App) SetAutoSave(enabled bool) {
	a.mu.Lock()
	a.autoSave = enabled
	a.mu.Unlock()
	if enabled {
		a.emitLog("INFO", "Auto-save enabled")
	} else {
		a.emitLog("INFO", "Auto-save disabled")
	}
}

// GetAutoSave returns the current auto-save state
func (a *App) GetAutoSave() bool {
	a.mu.Lock()
	defer a.mu.Unlock()
	return a.autoSave
}

// SaveFrameToFile saves the provided base64 image data to a file
func (a *App) SaveFrameToFile(imageB64 string, filename string) SaveResult {
	a.mu.Lock()
	saveDir := a.saveDirectory
	a.mu.Unlock()

	if saveDir == "" {
		return SaveResult{Success: false, Error: "No save directory set"}
	}

	// Decode base64 image
	imageData, err := base64.StdEncoding.DecodeString(imageB64)
	if err != nil {
		return SaveResult{Success: false, Error: fmt.Sprintf("Failed to decode image: %v", err)}
	}

	// Create full path
	if filename == "" {
		filename = fmt.Sprintf("capture_%s.jpg", time.Now().Format("20060102_150405.000"))
	}
	filePath := filepath.Join(saveDir, filename)

	// Write file
	if err := os.WriteFile(filePath, imageData, 0644); err != nil {
		return SaveResult{Success: false, Error: fmt.Sprintf("Failed to write file: %v", err)}
	}

	a.emitLog("INFO", fmt.Sprintf("Saved frame to: %s", filePath))
	return SaveResult{Success: true, FilePath: filePath}
}

// SelectSaveDirectory opens a directory picker dialog
func (a *App) SelectSaveDirectory() (string, error) {
	dir, err := runtime.OpenDirectoryDialog(a.ctx, runtime.OpenDialogOptions{
		Title:            "Select Save Directory",
		ShowHiddenFiles:  false,
	})
	if err != nil {
		return "", err
	}
	if dir != "" {
		a.mu.Lock()
		a.saveDirectory = dir
		a.mu.Unlock()
		a.emitLog("INFO", fmt.Sprintf("Save directory set to: %s", dir))
	}
	return dir, nil
}

// TriggerCapture triggers a manual capture
// NOTE: TriggerCapture is not currently supported by the C API
func (a *App) TriggerCapture(scenarioID int) error {
	a.mu.Lock()
	client := a.client
	a.mu.Unlock()

	if client == nil {
		return fmt.Errorf("not connected")
	}

	// TODO: Trigger capture is not currently available in the C API
	a.emitLog("WARNING", "TriggerCapture not yet implemented in C API")
	return fmt.Errorf("trigger capture not supported")
}

// Callback handlers
// IMPORTANT: These are called from the SDK's C++ handler thread via CGo.
// They MUST return immediately to avoid blocking the SDK's response processing.
// All actual work is done in goroutines.

func (a *App) onTriggerCapture(result *itscam.CaptureResult) {
	// Copy the result data before spawning goroutine
	// (the result may be invalid after this callback returns)
	resultCopy := copyResult(result)
	go func() {
		a.handleCapture(resultCopy, string(FrameSourceTrigger))
	}()
}

func (a *App) onSnapshotCapture(result *itscam.CaptureResult) {
	// Copy the result data before spawning goroutine
	resultCopy := copyResult(result)
	go func() {
		a.handleCapture(resultCopy, string(FrameSourceSnapshot))
	}()
}

// copyResult creates a deep copy of the capture result
func copyResult(result *itscam.CaptureResult) *itscam.CaptureResult {
	if result == nil {
		return nil
	}
	// Copy JPEG data
	jpegCopy := make([]byte, len(result.JpegData))
	copy(jpegCopy, result.JpegData)
	// Copy plates
	platesCopy := make([]string, len(result.Plates))
	copy(platesCopy, result.Plates)
	return &itscam.CaptureResult{
		FrameInfo: result.FrameInfo, // struct copy
		JpegData:  jpegCopy,
		Plates:    platesCopy,
	}
}

func (a *App) handleCapture(result *itscam.CaptureResult, source string) {
	a.captureCount++
	info := result.FrameInfo

	// Encode image as base64 for frontend
	imageB64 := base64.StdEncoding.EncodeToString(result.JpegData)

	event := CaptureEvent{
		Source:         source,
		RequestID:      info.RequestID,
		FrameCount:     info.FrameCount,
		Width:          info.Width,
		Height:         info.Height,
		Timestamp:      info.Timestamp.ToISO8601(),
		Plates:         result.Plates,
		ImageB64:       imageB64,
		ImageSize:      len(result.JpegData),
		MultiExpIndex:  info.MultiExpIndex,
		MultiExpLength: info.MultiExpLength,
		Shutter:        info.Shutter,
		Gain:           info.Gain,
	}

	runtime.EventsEmit(a.ctx, "capture", event)

	// Auto-save if enabled
	a.mu.Lock()
	autoSave := a.autoSave
	saveDir := a.saveDirectory
	a.mu.Unlock()

	if autoSave && saveDir != "" {
		filename := fmt.Sprintf("%s_%s_frame%d.jpg", source, info.Timestamp.ToTime().Format("20060102_150405"), info.FrameCount)
		go func() {
			result := a.SaveFrameToFile(imageB64, filename)
			if !result.Success {
				a.emitLog("ERROR", fmt.Sprintf("Auto-save failed: %s", result.Error))
			}
		}()
	}
}

func (a *App) onDisconnect(reason string) {
	// Copy reason string and process async
	reasonCopy := reason
	go func() {
		a.mu.Lock()
		a.connected = false
		a.mu.Unlock()

		a.emitLog("WARNING", fmt.Sprintf("Disconnected: %s", reasonCopy))
		a.emitConnectionStatus(false, "Disconnected", reasonCopy)
	}()
}

func (a *App) onConnectionState(state itscam.ConnectionState, reason string) {
	// Copy values and process async
	stateCopy := state
	reasonCopy := reason
	go func() {
		stateStr := stateCopy.String()
		connected := stateCopy == itscam.StateConnected || stateCopy == itscam.StateReconnected

		a.mu.Lock()
		a.connected = connected
		a.mu.Unlock()

		a.emitConnectionStatus(connected, stateStr, reasonCopy)
	}()
}

func (a *App) onLog(level itscam.LogLevel, message string) {
	// Copy values and process async
	levelCopy := level
	messageCopy := message
	go func() {
		a.emitLog(levelCopy.String(), messageCopy)
	}()
}

// Helper methods

func (a *App) emitLog(level, message string) {
	entry := LogEntry{
		Level:     level,
		Message:   message,
		Timestamp: time.Now().Format("15:04:05.000"),
	}
	runtime.EventsEmit(a.ctx, "log", entry)
}

func (a *App) emitConnectionStatus(connected bool, state, message string) {
	status := ConnectionStatus{
		Connected: connected,
		State:     state,
		Message:   message,
	}
	runtime.EventsEmit(a.ctx, "connectionStatus", status)
}

/*
 * capture_example.go
 *
 * ITSCAM SDK Go Example
 *
 * This example demonstrates how to:
 * - Connect to an ITSCAM camera with auto-reconnection
 * - Authenticate (optional)
 * - Subscribe to trigger and snapshot events
 * - Receive trigger images via callbacks
 * - Capture snapshots
 * - Save captured images to disk
 *
 * Usage:
 *     go run capture_example.go <camera_ip> [password]
 *
 * Example:
 *     go run capture_example.go 192.168.1.100
 *     go run capture_example.go 192.168.1.100 1234
 *
 * Copyright (c) 2026 Pumatronix
 */

package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"sync/atomic"
	"time"

	"github.com/pumatronix/itscam-sdk-go/itscam"
)

var (
	triggerImageCount int64
	snapshotCount     int64
	outputDir         = "captures"
)

// log prints a message to stdout.
func log(msg string) {
	fmt.Println(msg)
}

// logErr prints an error message to stderr.
func logErr(msg string) {
	fmt.Fprintf(os.Stderr, "[ERROR] %s\n", msg)
}

// saveJpeg saves JPEG data to a file.
func saveJpeg(prefix, useCase string, rid uint64, expIdx int, data []byte) error {
	filename := fmt.Sprintf("%s_%s_%d_%d.jpg", prefix, useCase, rid, expIdx)
	filePath := filepath.Join(outputDir, filename)

	if err := os.WriteFile(filePath, data, 0644); err != nil {
		logErr("Cannot write " + filePath + ": " + err.Error())
		return err
	}
	log("Saved: " + filePath)
	return nil
}

// onTriggerImage handles trigger image events.
func onTriggerImage(result *itscam.CaptureResult, cameraIP string) {
	count := atomic.AddInt64(&triggerImageCount, 1)
	info := result.FrameInfo

	log(fmt.Sprintf("Received TRIGGER frame #%d, RID=%d, multiExp %d/%d, size=%d bytes",
		count, info.RequestID, info.MultiExpIndex+1, info.MultiExpLength, len(result.JpegData)))

	if len(result.Plates) > 0 {
		log(fmt.Sprintf("  Plates: %s", strings.Join(result.Plates, ", ")))
	}

	saveJpeg(cameraIP, "trigger", info.RequestID, info.MultiExpIndex, result.JpegData)
}

// onSnapshotImage handles snapshot image events.
func onSnapshotImage(result *itscam.CaptureResult, cameraIP string) {
	count := atomic.AddInt64(&snapshotCount, 1)
	info := result.FrameInfo

	log(fmt.Sprintf("Received SNAPSHOT frame #%d, RID=%d, multiExp %d/%d, size=%d bytes",
		count, info.RequestID, info.MultiExpIndex+1, info.MultiExpLength, len(result.JpegData)))

	saveJpeg(cameraIP, "snapshot", info.RequestID, info.MultiExpIndex, result.JpegData)
}

// onConnectionStateChanged handles connection state changes.
func onConnectionStateChanged(state itscam.ConnectionState, reason string) {
	log(fmt.Sprintf("[ConnState] %s: %s", state, reason))
}

// onLog handles SDK log messages.
func onLog(level itscam.LogLevel, message string) {
	if level == itscam.LogError {
		fmt.Fprintf(os.Stderr, "[SDK ERROR] %s\n", message)
	} else {
		fmt.Printf("[SDK] %s\n", message)
	}
}

func printUsage(progName string) {
	fmt.Fprintf(os.Stderr, "Usage: %s <camera_ip> [password]\n", progName)
	fmt.Fprintln(os.Stderr)
	fmt.Fprintln(os.Stderr, "  camera_ip   IP address of the ITSCAM camera (required)")
	fmt.Fprintln(os.Stderr, "  password    Authentication password (optional, default: none)")
	fmt.Fprintln(os.Stderr)
	fmt.Fprintln(os.Stderr, "Examples:")
	fmt.Fprintf(os.Stderr, "  %s 192.168.254.254\n", progName)
	fmt.Fprintf(os.Stderr, "  %s 192.168.254.254 1234\n", progName)
}

func main() {
	// --- Argument parsing ---
	if len(os.Args) < 2 || os.Args[1] == "--help" || os.Args[1] == "-h" {
		printUsage(os.Args[0])
		if len(os.Args) < 2 {
			os.Exit(1)
		}
		os.Exit(0)
	}

	cameraIP := os.Args[1]
	password := ""
	if len(os.Args) >= 3 {
		password = os.Args[2]
	}

	log("ITSCAM Client SDK Go Example")
	log("Camera: " + cameraIP)

	// Create output directory
	if err := os.MkdirAll(outputDir, 0755); err != nil {
		logErr("Failed to create output directory: " + err.Error())
		os.Exit(1)
	}

	// --- SDK setup ---
	client, err := itscam.NewClient()
	if err != nil {
		logErr("Failed to create client: " + err.Error())
		os.Exit(1)
	}
	defer client.Close()

	// Set log handler
	client.SetLogCallback(onLog)

	// --- Connection state callback ---
	client.SetConnectionStateCallback(onConnectionStateChanged)

	// --- Connect with auto-reconnection ---
	reconnect := itscam.AutoReconnectConfig{
		Enabled:     true,
		Interval:    3 * time.Second, // retry every 3 seconds
		MaxAttempts: 0,               // unlimited retries
	}

	log("Connecting...")
	if err := client.ConnectWithReconnect(cameraIP, 60000, 5*time.Second, reconnect); err != nil {
		logErr("Could not connect: " + err.Error())
		os.Exit(1)
	}
	log("Connected")

	// --- Authenticate (if password provided) ---
	if password != "" {
		log("Authenticating...")
		if err := client.Authenticate(password, 5*time.Second); err != nil {
			logErr("Authentication failed: " + err.Error())
			os.Exit(1)
		}
		log("Authenticated")
	}

	// --- Register callbacks BEFORE subscribe ---
	// This is important: callbacks must be set before subscribing
	client.SetCaptureCallback(func(result *itscam.CaptureResult) {
		onTriggerImage(result, cameraIP)
	})
	client.SetSnapshotCallback(func(result *itscam.CaptureResult) {
		onSnapshotImage(result, cameraIP)
	})

	// --- Subscribe to events ---
	captureConfig := itscam.DefaultCaptureSubscriptionConfig()

	log("Subscribing to events...")
	if err := client.SubscribeCaptures(captureConfig, 5*time.Second); err != nil {
		logErr("Failed to subscribe: " + err.Error())
		os.Exit(1)
	}
	log("Subscribed to trigger and snapshot events")

	// ========================================================================
	//  1. Snapshot capture
	// ========================================================================
	log("")
	log("=== Snapshot Capture ===")

	if err := client.RequestSnapshot(5 * time.Second); err != nil {
		logErr("Snapshot request failed: " + err.Error())
	} else {
		log("Snapshot requested (result will arrive via callback)")
	}

	// Wait a bit for snapshot callback
	time.Sleep(2 * time.Second)

	// ========================================================================
	//  2. Device info
	// ========================================================================
	log("")
	log("=== Device Info ===")

	activeProfile, err := client.GetActiveProfile(5 * time.Second)
	if err != nil {
		logErr("Failed to get active profile: " + err.Error())
	} else {
		log(fmt.Sprintf("  Active Profile: %d", activeProfile))
	}
	log(fmt.Sprintf("  Address: %s", client.Address()))
	log(fmt.Sprintf("  Port: %d", client.Port()))
	log(fmt.Sprintf("  Connected: %v", client.IsConnected()))

	// ========================================================================
	//  3. Wait for trigger images
	// ========================================================================
	log("")
	log("=== Waiting for Trigger Images ===")
	log("Waiting 15s for trigger images...")
	log("(Configure trigger to continuous mode on the camera to receive images)")

	time.Sleep(15 * time.Second)

	log(fmt.Sprintf("Received %d trigger image(s), %d snapshot(s)",
		atomic.LoadInt64(&triggerImageCount),
		atomic.LoadInt64(&snapshotCount)))

	log("")
	log("Done.")
}

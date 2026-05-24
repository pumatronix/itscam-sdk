# ITSCAM Go SDK

Pumatronix ITSCAM Camera Client SDK for Go.

## Requirements

- Go 1.21+
- Native library (libitscam_sdk.so or itscam_sdk.dll)
- GCC or compatible C compiler (for cgo)

## Installation

```bash
go get github.com/pumatronix/itscam-sdk-go/itscam
```

Ensure the native library is installed:
- Linux: Place `libitscam_sdk.so` in `/usr/lib` or set `LD_LIBRARY_PATH`
- Windows: Place `itscam_sdk.dll` in `PATH`

## Quick Start

```go
package main

import (
    "fmt"
    "log"
    "os"
    "time"

    "github.com/pumatronix/itscam-sdk-go/itscam"
)

func main() {
    // Create client
    client, err := itscam.NewClient("192.168.1.100", 50000)
    if err != nil {
        log.Fatal(err)
    }
    defer client.Close()

    // Connect with 5 second timeout
    if err := client.Connect(5 * time.Second); err != nil {
        log.Fatal(err)
    }

    // Authenticate
    if err := client.Authenticate("admin", "password"); err != nil {
        log.Fatal(err)
    }

    // Get device info
    fmt.Println("Version:", client.Version())
    fmt.Println("Serial:", client.Serial())

    // Set capture callback
    client.SetCaptureCallback(func(result *itscam.CaptureResult) {
        fmt.Printf("Captured frame %d: %dx%d, %d bytes\n",
            result.FrameInfo.FrameID,
            result.FrameInfo.Width,
            result.FrameInfo.Height,
            len(result.JpegData))

        // Save image
        filename := fmt.Sprintf("capture_%d.jpg", result.FrameInfo.FrameID)
        os.WriteFile(filename, result.JpegData, 0644)
    })

    // Subscribe to capture events
    client.SubscribeToEvents(itscam.AllCaptures())

    // Wait for events
    time.Sleep(60 * time.Second)
}
```

## Features

### Connection Management

```go
// Create and connect
client, _ := itscam.NewClient("192.168.1.100", 50000)
defer client.Close()

client.Connect(5 * time.Second)

// Enable auto-reconnect
client.EnableAutoReconnect(itscam.AutoReconnectConfig{
    Enabled:     true,
    Interval:    3 * time.Second,
    MaxAttempts: 10,
})

// Monitor connection state
client.SetConnectionStateCallback(func(state itscam.ConnectionState, reason string) {
    fmt.Printf("State: %s - %s\n", state, reason)
})
```

### Profile Management

```go
// Get profiles
count := client.ProfileCount()
active := client.ActiveProfile()

// Switch profile
client.SetActiveProfile(1)
```

### Capture Control

```go
// Manual trigger
client.TriggerCapture(1) // scenario 1

// Subscribe to specific scenarios
client.SubscribeToEvents(itscam.ForScenarios(1, 2, 3))
```

### Error Handling

```go
err := client.Connect(5 * time.Second)
if err != nil {
    switch err.(type) {
    case *itscam.ConnectionError:
        fmt.Println("Connection failed:", err)
    case *itscam.TimeoutError:
        fmt.Println("Timeout:", err)
    case *itscam.AuthError:
        fmt.Println("Auth failed:", err)
    default:
        fmt.Println("Error:", err)
    }
}
```

## API Reference

### Client Methods

| Method | Description |
|--------|-------------|
| `NewClient(host, port)` | Create new client |
| `Close()` | Release resources |
| `Connect(timeout)` | Connect to camera |
| `Disconnect()` | Disconnect from camera |
| `IsConnected()` | Check connection status |
| `Authenticate(user, pass)` | Authenticate |
| `Version()` | Get firmware version |
| `Serial()` | Get serial number |
| `ProfileCount()` | Get number of profiles |
| `ActiveProfile()` | Get active profile index |
| `SetActiveProfile(i)` | Set active profile |
| `TriggerCapture(scenario)` | Trigger capture |
| `SetCaptureCallback(cb)` | Set capture handler |
| `SetDisconnectCallback(cb)` | Set disconnect handler |
| `SetConnectionStateCallback(cb)` | Set state handler |
| `EnableAutoReconnect(config)` | Enable auto-reconnect |
| `SubscribeToEvents(sub)` | Subscribe to events |
| `SetLogLevel(level)` | Set log level |

### Utility Functions

| Function | Description |
|----------|-------------|
| `StoreFile(path, data)` | Store file with auto-mkdir |
| `CreateFolder(path)` | Create directory recursively |

## Building

```bash
# Ensure native library is in library path
export LD_LIBRARY_PATH=/path/to/sdk:$LD_LIBRARY_PATH

# Build
go build ./...

# Test
go test ./...
```

## Examples

### Console Example

A command-line example that connects to a camera, subscribes to events, and saves captured images:

```bash
cd examples
go run capture_example.go 192.168.1.100 50000
```

### GUI Example (Wails)

A desktop GUI application built with [Wails](https://wails.io/) that provides a visual interface for:
- Connecting to cameras
- Viewing live captures
- Displaying captured images with metadata
- Capture history with thumbnails

See [examples/gui/README.md](examples/gui/README.md) for build instructions.

```bash
cd examples/gui
./build-linux.sh    # Ubuntu 20.04
./build-windows.sh  # Windows (cross-compile)
```

## Cross-Compilation

For cross-compiling, you need the appropriate cross-compiler and target libraries:

```bash
# Linux ARM64
CGO_ENABLED=1 GOOS=linux GOARCH=arm64 CC=aarch64-linux-gnu-gcc go build

# Windows
CGO_ENABLED=1 GOOS=windows GOARCH=amd64 CC=x86_64-w64-mingw32-gcc go build
```

## License

Copyright (c) 2026 Pumatronix. All rights reserved.

# ITSCAM Viewer - Go GUI Example

A desktop GUI application built with [Wails](https://wails.io/) to demonstrate the ITSCAM SDK Go wrapper capabilities.

## Features

- Connect to ITSCAM cameras
- **Frame Source Selection**: Choose between trigger events, snapshot events, or both
- **Snapshot Requests**: Request on-demand snapshots from the camera
- View live capture events with real-time updates
- **Detailed Metadata Display**: View frame metadata including:
  - Source type (trigger/snapshot)
  - Request ID and frame count
  - Resolution and image size
  - Timestamp
  - Shutter speed and gain
  - Multi-exposure information
  - Detected license plates
- **Metadata Overlay**: Toggle an overlay on the image showing key metadata
- **Save Functionality**: 
  - Select save directory
  - Auto-save incoming frames
  - Manual save of current frame
- Show device information (address, port, active profile)
- Capture history with thumbnails and source badges
- Real-time logging

## Screenshots

The application provides a dark-themed interface with:
- Left panel: Connection settings, frame source selection, and save settings
- Center: Live image display with optional metadata overlay, metadata panel, and capture history
- Right panel: Real-time logs

## Prerequisites

### Ubuntu 20.04

```bash
# Go 1.21+
sudo snap install go --classic

# Wails CLI
go install github.com/wailsapp/wails/v2/cmd/wails@latest

# WebKit2GTK development libraries
sudo apt update
sudo apt install libwebkit2gtk-4.0-dev libgtk-3-dev

# Build tools
sudo apt install build-essential pkg-config
```

### Windows (native build)

1. Install [Go 1.21+](https://golang.org/dl/)
2. Install [Wails CLI](https://wails.io/docs/gettingstarted/installation):
   ```powershell
   go install github.com/wailsapp/wails/v2/cmd/wails@latest
   ```
3. Install [WebView2](https://developer.microsoft.com/en-us/microsoft-edge/webview2/) (usually pre-installed on Windows 10/11)

### Windows (cross-compile from Linux)

```bash
# MinGW-w64
sudo apt install gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64
```

## Building

### Using Docker (Recommended)

The easiest way to build is using the SDK Docker container:

```bash
# From the SDK root directory
cd /path/to/sdk

# Build for Linux
make docker-go-gui

# Build for Windows (cross-compile)
make docker-go-gui-windows
```

### Build for Ubuntu 20.04 (standalone)

```bash
# Make the script executable
chmod +x build-linux.sh

# Build
./build-linux.sh
```

The binary will be created at `build/bin/itscam-viewer`.

### Build for Windows (cross-compile)

```bash
# Make the script executable
chmod +x build-windows.sh

# Build
./build-windows.sh
```

The binary will be created at `build/bin/itscam-viewer.exe` along with `itscam_sdk.dll`.

### Development Mode

For development with hot-reload:

```bash
wails dev
```

## Running

### Linux

```bash
# Set library path
export LD_LIBRARY_PATH=../../../../../src/core/build/linux:$LD_LIBRARY_PATH

# Run
./build/bin/itscam-viewer
```

### Windows

Ensure `itscam_sdk.dll` is in the same directory as `itscam-viewer.exe`, then run:

```powershell
.\itscam-viewer.exe
```

## Usage

1. **Connect to Camera**
   - Enter the camera IP address and port (default: 60000)
   - Enter the authentication password
   - Click "Connect"

2. **Subscribe to Captures**
   - Once connected, click "Subscribe" to start receiving capture events
   - Images will appear in the main display area

3. **Trigger Capture**
   - Select a scenario ID (0-15)
   - Click "Trigger Capture" to manually trigger a capture

4. **View History**
   - Captured images appear in the history carousel
   - Click on a thumbnail to view it in the main display

## Project Structure

```
gui/
├── app.go              # Go backend bindings
├── main.go             # Wails application entry point
├── go.mod              # Go module definition
├── wails.json          # Wails configuration
├── build-linux.sh      # Linux build script
├── build-windows.sh    # Windows cross-compile script
├── README.md           # This file
└── frontend/
    ├── index.html      # Main HTML
    ├── styles.css      # Styles
    └── app.js          # Frontend JavaScript
```

## API Reference

The Go backend exposes the following methods to the frontend:

| Method | Description |
|--------|-------------|
| `Connect(ip, port, password)` | Connect to camera |
| `Disconnect()` | Disconnect from camera |
| `Subscribe()` | Start receiving capture events |
| `Unsubscribe()` | Stop receiving capture events |
| `TriggerCapture(scenarioID)` | Trigger a manual capture |
| `GetDeviceInfo()` | Get device information |
| `GetConnectionStatus()` | Get connection status |

## Events

The backend emits the following events:

| Event | Data | Description |
|-------|------|-------------|
| `capture` | `CaptureEvent` | New capture received |
| `connectionStatus` | `ConnectionStatus` | Connection state changed |
| `log` | `LogEntry` | Log message |

## Troubleshooting

### "undefined reference to ITSCAM_xxx"
The ITSCAM SDK library is not found. Build the SDK first:
```bash
cd ../../../../
make linux  # or make windows
```

### WebKit2GTK errors on Linux
Install the development libraries:
```bash
sudo apt install libwebkit2gtk-4.0-dev
```

### Application doesn't start on Windows
Ensure `itscam_sdk.dll` is in the same directory as the executable.

## License

Copyright (c) 2026 Pumatronix

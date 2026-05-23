# ITSCAM Client SDK - Examples

## Prerequisites

- C++14 compiler (GCC 5+, Clang 3.4+)
- nlohmann/json (header-only, already present in the repository)
- Linux with POSIX sockets

## Building

From this directory:

```bash
make
```

If nlohmann/json is installed in a non-default location:

```bash
make NLOHMANN_JSON_DIR=/path/to/nlohmann/json/single_include
```

## Examples

### itscam_usage_example

Demonstrates the full SDK workflow:

1. Connect and authenticate to an ITSCAM camera
2. Subscribe to snapshot and trigger capture events with one helper
3. Customize JPEG/IMGPKG settings only when the defaults are not enough
4. Capture a snapshot with overlay text (single call: trigger + wait for image)
5. Configure scenario overlays and crop regions, then capture per-scenario
6. Set trigger to **continuous mode**, wait for trigger images via callback,
   then restore the original trigger config
7. Read exposure settings and device info

**Usage:**

```bash
./itscam_usage_example <camera_ip> [password]
```

**Arguments:**

| Argument    | Required | Description                            |
|-------------|----------|----------------------------------------|
| `camera_ip` | yes      | IP address of the ITSCAM camera        |
| `password`  | no       | Authentication password (if configured)|

**Examples:**

```bash
# No authentication
./itscam_usage_example 192.168.254.254

# With authentication
./itscam_usage_example 192.168.254.254 1234

# Using the Makefile helper
make run IP=192.168.254.254 PASS=1234
```

### itscam_trigger_recorder

A standalone trigger event recorder that saves images to disk. This example
is statically linked and designed to run continuously until interrupted.

**Features:**

- Automatic reconnection to camera on connection loss
- Configurable filename format with timestamp and metadata placeholders
- Datetime extracted from camera JPEG metadata (COM section)
- Runs until Ctrl+C (SIGINT/SIGTERM)
- Creates output directories automatically

**Usage:**

```bash
./itscam_trigger_recorder <camera_ip> <output_folder> [options]
```

**Arguments:**

| Argument         | Required | Description                                    |
|------------------|----------|------------------------------------------------|
| `camera_ip`      | yes      | IP address of the ITSCAM camera                |
| `output_folder`  | yes      | Directory to save trigger images               |
| `-p, --password` | no       | Authentication password                        |
| `-f, --format`   | no       | Filename format (default: `{ip}_trigger_{datetime}_{exp}.jpg`) |
| `-q, --quality`  | no       | JPEG quality 1-100 (default: camera setting)   |
| `-r, --reconnect-interval` | no | Reconnect interval in ms (default: 3000) |

**Filename format placeholders:**

| Placeholder  | Description                       |
|-------------|-----------------------------------|
| `{ip}`      | Camera IP (dots → underscores)    |
| `{rid}`     | Request ID                        |
| `{frame}`   | Frame count                       |
| `{exp}`     | Multi-exposure index (0-based)    |
| `{explen}`  | Multi-exposure length             |
| `{datetime}`| Full datetime: YYYYMMDD_HHMMSS_mmm|
| `{date}`    | Date only: YYYYMMDD               |
| `{time}`    | Time only: HHMMSS_mmm             |
| `{year}`, `{month}`, `{day}`, `{hour}`, `{min}`, `{sec}`, `{msec}` | Individual datetime components |
| `{count}`   | Global image counter              |

**Examples:**

```bash
# Basic usage - save to /tmp/triggers folder
./itscam_trigger_recorder 192.168.254.254 /tmp/triggers

# With authentication
./itscam_trigger_recorder 192.168.254.254 ./images -p 1234

# Custom filename format
./itscam_trigger_recorder 192.168.254.254 ./images -f "cam1_{datetime}_{exp}.jpg"

# Organize by date in subdirectories
./itscam_trigger_recorder 192.168.254.254 ./images -f "{date}/{rid}_{exp}.jpg"

# High quality images with fast reconnect
./itscam_trigger_recorder 192.168.254.254 ./images -q 95 -r 1000
```

## File Structure

```
examples/
  Makefile                        Build system
  README.md                       This file
  itscam_sdk_example.cpp          General SDK usage example
  itscam_rest_example.cpp         REST API example
  itscam_trigger_recorder.cpp     Trigger recorder (static linked)
```

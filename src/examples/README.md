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

### itscam_snapshot_to_freeflow

Demonstrates switching between the two main ITSCAM operational modes:

- **Snapshot mode**: on-demand image capture via `snapshot.cgi` (CGI client)
- **Freeflow mode**: continuous trigger with majority voting and REST API
  Client (RAC) integration for automatic plate dispatch

When switching to freeflow, the example configures a night profile with
2 multi-exposure steps at different flash power levels, enables analytics
majority voting, and sets up a RAC server with a JSON body template
containing plate, timestamp, and base64-encoded image.

**Usage:**

```bash
./itscam_snapshot_to_freeflow <host> <user> <password> [options]
```

**Arguments:**

| Argument              | Required | Description                                         |
|-----------------------|----------|-----------------------------------------------------|
| `host`                | yes      | Camera hostname or IP                               |
| `user`                | yes      | REST API username                                   |
| `password`            | yes      | REST API password                                   |
| `--https`             | no       | Use HTTPS instead of HTTP                           |
| `--insecure`          | no       | Skip TLS certificate verification                   |
| `--rac-host <host>`   | no       | RAC destination host (default: localhost)            |
| `--rac-port <port>`   | no       | RAC destination port (default: 8080)                |
| `--rac-path <path>`   | no       | RAC destination path (default: /api/captures)       |
| `--profile-name <n>`  | no       | Night profile name to configure (default: Noturno)  |
| `--duration <sec>`    | no       | Freeflow run duration in seconds (default: 30)      |

**Examples:**

```bash
# Basic usage with defaults
./itscam_snapshot_to_freeflow 192.168.254.254 admin 1234

# Custom RAC endpoint and longer freeflow duration
./itscam_snapshot_to_freeflow 192.168.254.254 admin 1234 \
    --rac-host 10.0.0.50 --rac-port 9090 --duration 60

# Specify a different profile name
./itscam_snapshot_to_freeflow 192.168.254.254 admin 1234 \
    --profile-name "Night"

# Using HTTPS
./itscam_snapshot_to_freeflow camera.example.com admin secret --https
```

**Testing with a local RAC receiver:**

The RAC service on the camera will POST JSON payloads to the configured
endpoint.  To verify the integration you can spin up a minimal Python HTTP
server that prints incoming requests:

```bash
# rac_test_server.py — paste or save this script, then run it
python3 -c "
from http.server import HTTPServer, BaseHTTPRequestHandler
import json, sys

class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(length)
        try:
            data = json.loads(body)
            plate = data.get('plate', '???')
            ts    = data.get('timestamp', '')
            img   = data.get('image', '')
            print(f'  plate={plate}  ts={ts}  image={len(img)} chars (base64)')
        except Exception:
            print(f'  raw body: {body[:200]}')
        self.send_response(200)
        self.end_headers()
        self.wfile.write(b'ok')

port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
print(f'Listening on http://localhost:{port}/api/captures ...')
HTTPServer(('', port), Handler).serve_forever()
" 8080
```

Then, in another terminal, run the example pointing at localhost (the default):

```bash
./itscam_snapshot_to_freeflow 192.168.254.254 admin 1234
# Python variant:
python3 snapshot_to_freeflow_example.py 192.168.254.254 admin 1234
```

## File Structure

```
examples/
  Makefile                        Build system
  README.md                       This file
  itscam_sdk_example.cpp          General SDK usage example
  itscam_rest_example.cpp         REST API example
  itscam_trigger_recorder.cpp     Trigger recorder (static linked)
  itscam_snapshot_to_freeflow.cpp Snapshot-to-freeflow swap example
```

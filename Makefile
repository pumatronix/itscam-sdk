# ITSCAM SDK Top-Level Makefile
#
# Orchestrates building the core library, C++ examples, and wrapper examples.
#
# Usage:
#   make                Build core library and C++ examples (Linux)
#   make lib            Build core library only (Linux)
#   make examples       Build C++ examples only
#   make windows        Build core library for Windows (cross-compile)
#   make wrappers       Build wrapper language examples
#   make all            Build everything for all platforms
#   make clean          Clean all build artifacts
#
# Copyright (c) 2026 Pumatronix

.PHONY: all linux windows lib examples wrappers clean help
.PHONY: python-example go-example go-gui
.PHONY: install
.PHONY: docker-build docker-all docker-linux docker-windows docker-shell docker-go-gui

# Default target - build library and C++ examples
default: lib examples

# ============================================================================
#  Core Library
# ============================================================================

lib:
	@echo "=== Building core library (Linux) ==="
	$(MAKE) -C core linux

linux: lib

windows:
	@echo "=== Building core library (Windows) ==="
	$(MAKE) -C core windows

# ============================================================================
#  C++ Examples
# ============================================================================

examples: lib
	@echo "=== Building C++ examples ==="
	$(MAKE) -C examples

# ============================================================================
#  Wrapper Examples
# ============================================================================

wrappers: python-example go-example

python-example:
	@echo "=== Python example ready ==="
	@echo "Run: python wrappers/python/examples/capture_example.py <camera_ip>"
	@echo "Or:  cd wrappers/python && pip install -e . && python examples/capture_example.py <camera_ip>"

go-example: lib
	@echo "=== Building Go example ==="
	@if command -v go > /dev/null; then \
		cd wrappers/go/examples && \
		CGO_LDFLAGS="-L$(CURDIR)/core/build/linux" \
		CGO_CFLAGS="-I$(CURDIR)/core" \
		go build -o capture_example capture_example.go; \
		echo "Run: LD_LIBRARY_PATH=$(CURDIR)/core/build/linux wrappers/go/examples/capture_example <camera_ip>"; \
	else \
		echo "Go not found. Install go to build Go examples."; \
	fi

go-gui: lib
	@echo "=== Building Go GUI example (Wails) for Linux (static) ==="
	@if command -v wails > /dev/null; then \
		export CGO_CFLAGS="-I$(CURDIR)/core" && \
		export LD_LIBRARY_PATH="$(CURDIR)/core/build/linux:$$LD_LIBRARY_PATH" && \
		cd wrappers/go/examples/gui && \
		go mod tidy && \
		mkdir -p build/bin/linux && \
		wails build -tags static -o itscam-viewer && \
		mv build/bin/itscam-viewer build/bin/linux/; \
		echo "Output: wrappers/go/examples/gui/build/bin/linux/"; \
		ls -lh build/bin/linux/; \
	else \
		echo "Wails not found. Install with: go install github.com/wailsapp/wails/v2/cmd/wails@latest"; \
	fi

go-gui-windows: windows
	@echo "=== Building Go GUI example (Wails) for Windows (static) ==="
	@if command -v wails > /dev/null; then \
		export CGO_ENABLED=1 && \
		export GOOS=windows && \
		export GOARCH=amd64 && \
		export CC=x86_64-w64-mingw32-gcc && \
		export CXX=x86_64-w64-mingw32-g++ && \
		export CGO_CFLAGS="-I$(CURDIR)/core" && \
		cd wrappers/go/examples/gui && \
		go mod tidy && \
		mkdir -p build/bin/windows && \
		wails build -tags static -platform windows/amd64 -o itscam-viewer.exe && \
		mv build/bin/itscam-viewer.exe build/bin/windows/; \
		echo "Output: wrappers/go/examples/gui/build/bin/windows/"; \
		ls -lh build/bin/windows/; \
	else \
		echo "Wails not found. Install with: go install github.com/wailsapp/wails/v2/cmd/wails@latest"; \
	fi

# ============================================================================
#  Docker Build
# ============================================================================

DOCKER_IMAGE := itscam-sdk-builder
DOCKER_RUN := docker run --rm -v $(CURDIR):/sdk -u $(shell id -u):$(shell id -g)

docker-build:
	@echo "=== Building Docker image ==="
	docker build -t $(DOCKER_IMAGE) .

docker-all: docker-build
	@echo "=== Building all inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make all

docker-linux: docker-build
	@echo "=== Building Linux library inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make lib

docker-windows: docker-build
	@echo "=== Cross-compiling for Windows inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make windows

docker-shell: docker-build
	@echo "=== Opening interactive shell in Docker ==="
	docker run --rm -it -v $(CURDIR):/sdk -u $(shell id -u):$(shell id -g) $(DOCKER_IMAGE) bash

docker-go-gui: docker-build
	@echo "=== Building Go GUI (Wails) inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make go-gui

docker-go-gui-windows: docker-build
	@echo "=== Building Go GUI for Windows inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make go-gui-windows

# ============================================================================
#  All Platforms
# ============================================================================

all: linux windows examples wrappers
	@echo "=== Build complete ==="

# ============================================================================
#  Installation
# ============================================================================

install:
	$(MAKE) -C core install

# ============================================================================
#  Cleanup
# ============================================================================

clean:
	@echo "=== Cleaning all build artifacts ==="
	$(MAKE) -C core clean
	$(MAKE) -C examples clean
	@rm -f wrappers/go/examples/capture_example
	@rm -rf wrappers/go/examples/gui/build
	@echo "Clean complete."

# ============================================================================
#  Help
# ============================================================================

help:
	@echo "ITSCAM SDK Build System"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Library targets:"
	@echo "  lib             Build core library (Linux) - default"
	@echo "  linux           Same as 'lib'"
	@echo "  windows         Build core library (Windows cross-compile)"
	@echo ""
	@echo "Example targets:"
	@echo "  examples        Build C++ examples"
	@echo "  wrappers        Build all wrapper examples"
	@echo "  python-example  Show Python example instructions"
	@echo "  go-example      Build Go example"
	@echo "  go-gui          Build Go GUI example (Wails) for Linux"
	@echo "  go-gui-windows  Build Go GUI example for Windows (cross-compile)"
	@echo ""
	@echo "Docker targets:"
	@echo "  docker-build    Build the Docker image"
	@echo "  docker-all      Build everything inside Docker"
	@echo "  docker-linux    Build Linux library inside Docker"
	@echo "  docker-windows  Cross-compile for Windows inside Docker"
	@echo "  docker-go-gui   Build Go GUI inside Docker (Linux)"
	@echo "  docker-go-gui-windows  Build Go GUI for Windows inside Docker"
	@echo "  docker-shell    Open interactive shell in Docker"
	@echo ""
	@echo "Other targets:"
	@echo "  all             Build everything"
	@echo "  install         Install library to system"
	@echo "  clean           Remove all build artifacts"
	@echo "  help            Show this help"
	@echo ""
	@echo "Directory structure:"
	@echo "  core/           C/C++ SDK library source"
	@echo "  examples/       C++ example applications"
	@echo "  wrappers/"
	@echo "    python/       Python binding and examples"
	@echo "    go/           Go binding and examples"

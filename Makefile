# ITSCAM SDK Top-Level Makefile
#
# Orchestrates building the core library, C++ examples, and wrapper
# examples.  All source code lives under src/ -- this Makefile delegates
# to src/core/Makefile and src/examples/Makefile and drives the wrapper
# build tools (dotnet, go, python).
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
.PHONY: python-example python-rest-example python-cgi-example
.PHONY: go-example go-rest-example go-cgi-example go-gui
.PHONY: csharp csharp-pack csharp-examples csharp-examples-publish
.PHONY: csharp-mjpeg-grabber-example csharp-software-trigger-example
.PHONY: install
.PHONY: version sdk-dist sdk-dist-clean docker-sdk-dist
.PHONY: docker-build docker-all docker-linux docker-windows docker-shell docker-go-gui
.PHONY: docker-csharp docker-csharp-examples docker-csharp-examples-publish
.PHONY: regression-examples docker-regression-examples
.PHONY: docs-api docs-api-cpp docs-api-python docs-api-csharp docs-api-go
.PHONY: docs-api-clean docs-sync docs-sync-check
.PHONY: docs-site docker-docs-api docker-docs-api-cpp docker-docs-site

# Source-tree root.  Every subordinate path below is relative to $(SRC_DIR).
SRC_DIR := src

VERSION_SCRIPT := tools/version/gen-version.sh
VERSION_MK := tools/version/sdk-version.mk
-include $(VERSION_MK)

# Docker runs bind-mount the repo; always map the caller's uid/gid so
# generated artefacts stay owned by the host user (never root).
DOCKER_IMAGE := itscam-sdk-builder
DOCKER_UID := $(shell id -u)
DOCKER_GID := $(shell id -g)
DOCKER_RUN := docker run --rm \
	-v $(CURDIR):/sdk \
	-w /sdk \
	-u $(DOCKER_UID):$(DOCKER_GID) \
	-e HOME=/tmp \
	-e XDG_CACHE_HOME=/tmp/.cache \
	-e NPM_CONFIG_CACHE=/tmp/.npm \
	-e DOTNET_CLI_HOME=/tmp/dotnet
DOCKER_RUN_IT := $(DOCKER_RUN) -it

# Default target - build library and C++ examples
default: lib examples

# ============================================================================
#  Version metadata (git tag + commit + build date)
# ============================================================================

version:
	@$(VERSION_SCRIPT)

# ============================================================================
#  Core Library
# ============================================================================

lib: version
	@echo "=== Building core library (Linux) ==="
	$(MAKE) -C $(SRC_DIR)/core linux

linux: lib

windows:
	@echo "=== Building core library (Windows) ==="
	$(MAKE) -C $(SRC_DIR)/core windows

# ============================================================================
#  C++ Examples
# ============================================================================

examples: lib
	@echo "=== Building C++ examples ==="
	$(MAKE) -C $(SRC_DIR)/examples

# ============================================================================
#  Wrapper Examples
# ============================================================================

wrappers: python-example go-example csharp-examples

python-example:
	@echo "=== Python example ready ==="
	@echo "Run: python $(SRC_DIR)/wrappers/python/examples/capture_example.py <camera_ip>"
	@echo "Or:  cd $(SRC_DIR)/wrappers/python && pip install -e . && python examples/capture_example.py <camera_ip>"

python-rest-example:
	@echo "=== Python REST example ready ==="
	@echo "Run: python $(SRC_DIR)/wrappers/python/examples/rest_example.py <host> <user> <pass>"

python-cgi-example:
	@echo "=== Python CGI snapshot example ready ==="
	@echo "Run: python $(SRC_DIR)/wrappers/python/examples/cgi_snapshot_example.py <host> [--user U --password P]"

go-example: lib
	@echo "=== Building Go example ==="
	@if command -v go > /dev/null; then \
		cd $(SRC_DIR)/wrappers/go/examples && \
		CGO_LDFLAGS="-L$(CURDIR)/$(SRC_DIR)/core/build/linux" \
		CGO_CFLAGS="-I$(CURDIR)/$(SRC_DIR)/core" \
		go build -o capture_example capture_example.go; \
		echo "Run: LD_LIBRARY_PATH=$(CURDIR)/$(SRC_DIR)/core/build/linux $(SRC_DIR)/wrappers/go/examples/capture_example <camera_ip>"; \
	else \
		echo "Go not found. Install go to build Go examples."; \
	fi

go-rest-example: lib
	@echo "=== Building Go REST example ==="
	@if command -v go > /dev/null; then \
		cd $(SRC_DIR)/wrappers/go/examples && \
		CGO_LDFLAGS="-L$(CURDIR)/$(SRC_DIR)/core/build/linux" \
		CGO_CFLAGS="-I$(CURDIR)/$(SRC_DIR)/core" \
		go build -o rest_example rest_example.go; \
	else \
		echo "Go not found. Install go to build Go examples."; \
	fi

go-cgi-example: lib
	@echo "=== Building Go CGI snapshot example ==="
	@if command -v go > /dev/null; then \
		cd $(SRC_DIR)/wrappers/go/examples && \
		CGO_LDFLAGS="-L$(CURDIR)/$(SRC_DIR)/core/build/linux" \
		CGO_CFLAGS="-I$(CURDIR)/$(SRC_DIR)/core" \
		go build -o cgi_snapshot_example cgi_snapshot_example.go; \
	else \
		echo "Go not found. Install go to build Go examples."; \
	fi

# ============================================================================
#  C# Wrapper
# ============================================================================

csharp: lib
	@echo "=== Building C# wrapper ==="
	@if command -v dotnet > /dev/null; then \
		cd $(SRC_DIR)/wrappers/csharp && \
		dotnet build -c Release Itscam.Sdk/Itscam.Sdk.csproj; \
		echo "C# build done: $(SRC_DIR)/wrappers/csharp/Itscam.Sdk/bin/Release/"; \
	else \
		echo "dotnet not found. Install .NET 8 SDK to build the C# wrapper."; \
	fi

# Produce a NuGet package containing native binaries for every platform
# in $(SRC_DIR)/core/build/<rid>/.  Builds the Linux artefacts first;
# cross-compiles for Windows when MinGW is available.  Optional ARM
# toolchains may be wired in by extending the lib-arm / lib-arm64
# targets below.
csharp-pack: lib
	@echo "=== Packing C# wrapper ==="
	@if ! command -v dotnet > /dev/null; then \
		echo "dotnet not found. Install .NET 8 SDK first."; exit 1; \
	fi
	@if command -v x86_64-w64-mingw32-g++ > /dev/null || \
	    command -v i686-w64-mingw32-g++ > /dev/null; then \
		$(MAKE) windows; \
	fi
	@rm -rf $(CURDIR)/$(SRC_DIR)/wrappers/csharp/nupkg
	@mkdir -p $(CURDIR)/$(SRC_DIR)/wrappers/csharp/nupkg
	cd $(SRC_DIR)/wrappers/csharp && \
	dotnet pack -c Release Itscam.Sdk/Itscam.Sdk.csproj \
	    -o $(CURDIR)/$(SRC_DIR)/wrappers/csharp/nupkg

# NuGet with Linux native binary only (used by sdk-dist on linux-x64).
csharp-pack-linux: lib
	@echo "=== Packing C# wrapper (Linux native only) ==="
	@if ! command -v dotnet > /dev/null; then \
		echo "dotnet not found. Install .NET 8 SDK first."; exit 1; \
	fi
	@rm -rf $(CURDIR)/$(SRC_DIR)/wrappers/csharp/nupkg
	@mkdir -p $(CURDIR)/$(SRC_DIR)/wrappers/csharp/nupkg
	cd $(SRC_DIR)/wrappers/csharp && \
	dotnet pack -c Release Itscam.Sdk/Itscam.Sdk.csproj \
	    -o $(CURDIR)/$(SRC_DIR)/wrappers/csharp/nupkg

# Build the SDK library + all example projects in one solution-level pass.
csharp-examples: lib
	@echo "=== Building C# wrapper and all examples ==="
	@if command -v dotnet > /dev/null; then \
		cd $(SRC_DIR)/wrappers/csharp && \
		dotnet build -c Release Itscam.Sdk.sln; \
		echo "C# examples built: $(SRC_DIR)/wrappers/csharp/examples/*/bin/Release/"; \
	else \
		echo "dotnet not found. Install .NET 8 SDK to build C# examples."; \
		echo "Use 'make docker-csharp-examples' to build inside the Docker container."; \
		exit 1; \
	fi

# Runtime identifier for self-contained publish.  Override on the command
# line if targeting a different architecture, e.g. CSHARP_RID=linux-arm64.
CSHARP_RID ?= linux-x64

# Publish all example executables as self-contained single-file binaries.
# Each output lands in examples/<Name>/bin/Release/net8.0/<rid>/publish/.
# The resulting binary has no .NET runtime dependency.
csharp-examples-publish: lib
	@echo "=== Publishing C# examples (self-contained, RID=$(CSHARP_RID)) ==="
	@if command -v dotnet > /dev/null; then \
		dotnet publish -c Release -r $(CSHARP_RID) --self-contained true \
		    -p:PublishSingleFile=true \
		    $(SRC_DIR)/wrappers/csharp/examples/CaptureExample/CaptureExample.csproj && \
		dotnet publish -c Release -r $(CSHARP_RID) --self-contained true \
		    -p:PublishSingleFile=true \
		    $(SRC_DIR)/wrappers/csharp/examples/MjpegGrabberExample/MjpegGrabberExample.csproj && \
		dotnet publish -c Release -r $(CSHARP_RID) --self-contained true \
		    -p:PublishSingleFile=true \
		    $(SRC_DIR)/wrappers/csharp/examples/SoftwareTriggerSnapshotExample/SoftwareTriggerSnapshotExample.csproj && \
		dotnet publish -c Release -r $(CSHARP_RID) --self-contained true \
		    -p:PublishSingleFile=true \
		    $(SRC_DIR)/wrappers/csharp/examples/BinaryCaptureExample/BinaryCaptureExample.csproj; \
		echo "Binaries in examples/*/bin/Release/net8.0/$(CSHARP_RID)/publish/"; \
	else \
		echo "dotnet not found. Install .NET 8 SDK to publish C# examples."; \
		echo "Use 'make docker-csharp-examples-publish' to publish inside the Docker container."; \
		exit 1; \
	fi

# Individual example convenience targets (depend on csharp-examples so the
# solution is always up-to-date before printing the run instructions).
csharp-mjpeg-grabber-example: csharp-examples
	@echo "Run: cd $(SRC_DIR)/wrappers/csharp/examples/MjpegGrabberExample"
	@echo "     dotnet run -- <host> --user admin --password 1234 [--duration 10]"

csharp-software-trigger-example: csharp-examples
	@echo "Run: cd $(SRC_DIR)/wrappers/csharp/examples/SoftwareTriggerSnapshotExample"
	@echo "     dotnet run -- <host> [--count 20 --interval 500]"
	@echo "     # add --user admin --password 1234 to also run REST configuration"

# ============================================================================
#  Live-camera regression (all non-GUI examples)
# ============================================================================

# Camera under test.  Override on the command line:
#   make regression-examples CAMERA_IP=192.168.254.254 CAMERA_USER=admin CAMERA_PASS=secret
CAMERA_IP ?=
CAMERA_USER ?= admin
CAMERA_PASS ?=

regression-examples:
	@if [ -z "$(CAMERA_IP)" ]; then \
		echo "Set CAMERA_IP, e.g.: make regression-examples CAMERA_IP=192.168.254.254"; \
		exit 1; \
	fi
	$(CURDIR)/tools/regression/run-examples.sh \
	    "$(CAMERA_IP)" "$(CAMERA_USER)" "$(CAMERA_PASS)"

docker-regression-examples: docker-build
	@if [ -z "$(CAMERA_IP)" ]; then \
		echo "Set CAMERA_IP, e.g.: make docker-regression-examples CAMERA_IP=192.168.254.254"; \
		exit 1; \
	fi
	$(DOCKER_RUN) --network host $(DOCKER_IMAGE) \
	    /sdk/tools/regression/run-examples.sh \
	    "$(CAMERA_IP)" "$(CAMERA_USER)" "$(CAMERA_PASS)"

go-gui: lib
	@echo "=== Building Go GUI example (Wails) for Linux (static) ==="
	@if command -v wails > /dev/null; then \
		export CGO_CFLAGS="-I$(CURDIR)/$(SRC_DIR)/core" && \
		export LD_LIBRARY_PATH="$(CURDIR)/$(SRC_DIR)/core/build/linux:$$LD_LIBRARY_PATH" && \
		cd $(SRC_DIR)/wrappers/go/examples/gui && \
		go mod tidy && \
		mkdir -p build/bin/linux && \
		wails build -tags static -o itscam-viewer && \
		mv build/bin/itscam-viewer build/bin/linux/; \
		echo "Output: $(SRC_DIR)/wrappers/go/examples/gui/build/bin/linux/"; \
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
		export CGO_CFLAGS="-I$(CURDIR)/$(SRC_DIR)/core" && \
		cd $(SRC_DIR)/wrappers/go/examples/gui && \
		go mod tidy && \
		mkdir -p build/bin/windows && \
		wails build -tags static -platform windows/amd64 -o itscam-viewer.exe && \
		mv build/bin/itscam-viewer.exe build/bin/windows/; \
		echo "Output: $(SRC_DIR)/wrappers/go/examples/gui/build/bin/windows/"; \
		ls -lh build/bin/windows/; \
	else \
		echo "Wails not found. Install with: go install github.com/wailsapp/wails/v2/cmd/wails@latest"; \
	fi

# ============================================================================
#  Code generation (typed REST helpers from OpenAPI)
# ============================================================================
#
# `make codegen` regenerates language-specific typed REST helpers from
# tools/codegen/spec/default.yaml.  Override the spec path with SPEC=... to
# target a custom OpenAPI document (e.g. one pulled from a newer firmware
# release), or OUT_DIR=... to write outputs elsewhere for offline review.
#
# The generator is implemented in Node.js (tools/codegen/codegen.mjs), so
# Node 18+ is required locally.  Use `make docker-codegen` to run inside the
# project's reproducible builder image when Node isn't available.
#
# Generated files are checked into the repo -- consumers never need to run
# codegen during a normal build.

CODEGEN_DIR := tools/codegen
NODE ?= node
NPM ?= npm
SPEC ?=
OUT_DIR ?=

# Internal helper -- forward SPEC / OUT_DIR overrides to codegen.mjs.
CODEGEN_ARGS = $(if $(SPEC),--spec $(SPEC),) $(if $(OUT_DIR),--out-dir $(OUT_DIR),)

# Make sure tools/codegen has its node_modules in place before running.
codegen-install:
	@if [ ! -d $(CODEGEN_DIR)/node_modules ]; then \
		echo "=== Installing codegen Node dependencies ==="; \
		cd $(CODEGEN_DIR) && $(NPM) install --no-audit --no-fund; \
	fi

codegen: codegen-install
	@echo "=== Generating typed REST helpers ==="
	cd $(CODEGEN_DIR) && $(NODE) codegen.mjs $(CODEGEN_ARGS)

codegen-check: codegen-install
	@echo "=== Verifying generated REST helpers match committed snapshot ==="
	cd $(CODEGEN_DIR) && $(NODE) codegen.mjs --check $(CODEGEN_ARGS)

# ============================================================================
#  Documentation site (VitePress) and API reference (Doxygen, ...)
# ============================================================================
#
# `make docs-api-cpp` runs Doxygen on the public C/C++ headers under
# src/core/ and writes HTML to docs-site/content/public/api-ref/cpp/.
# VitePress copies that directory verbatim into the deployed site, so
# the generated reference is reachable at /api-ref/cpp/.
#
# Wrapper API references (Python via pdoc, C# via DocFX, Go via
# gomarkdoc) hang off `docs-api` and are added in their own targets.
#
# `make docs-sync` runs the VitePress content sync script standalone;
# `docs-sync-check` re-runs it and verifies that no source change was
# accidentally made to README.md, AGENTS.md, or docs/ that would alter
# the synced site output.

DOCS_SITE_DIR    := docs-site
DOXYGEN          ?= doxygen
DOXYFILE         := tools/docs/Doxyfile
DOCS_API_OUT     := $(DOCS_SITE_DIR)/content/public/api-ref

docs-api-cpp:
	@echo "=== Generating C/C++ API reference (Doxygen) ==="
	@command -v $(DOXYGEN) >/dev/null 2>&1 || { \
		echo "doxygen not found in PATH. Use 'make docker-docs-api-cpp' or 'apt install doxygen'." >&2; \
		exit 1; \
	}
	@mkdir -p $(DOCS_API_OUT)/cpp
	$(DOXYGEN) $(DOXYFILE)

# Python wrapper documentation. pdoc imports the package, which loads
# libitscam_sdk via ctypes -- so we depend on `lib` and prepend the
# build directory to LD_LIBRARY_PATH for the duration of the run.
PYTHON_WRAPPER_DIR := $(SRC_DIR)/wrappers/python
docs-api-python: lib
	@echo "=== Generating Python API reference (pdoc) ==="
	@command -v pdoc >/dev/null 2>&1 || { \
		echo "pdoc not found in PATH. 'pip install pdoc' or use 'make docker-docs-api-python'." >&2; \
		exit 1; \
	}
	@mkdir -p $(DOCS_API_OUT)/python
	@LD_LIBRARY_PATH=$(CURDIR)/$(SRC_DIR)/core/build/linux:$$LD_LIBRARY_PATH \
		PYTHONPATH=$(CURDIR)/$(PYTHON_WRAPPER_DIR):$$PYTHONPATH \
		pdoc itscam --output-directory $(DOCS_API_OUT)/python

# C# / .NET wrapper documentation. DocFX consumes the XML docs that the
# csproj already emits via <GenerateDocumentationFile>true</...>.
DOCFX ?= docfx
docs-api-csharp:
	@echo "=== Generating C# / .NET API reference (DocFX) ==="
	@command -v $(DOCFX) >/dev/null 2>&1 || { \
		echo "docfx not found in PATH. 'dotnet tool install -g docfx' or use 'make docker-docs-api-csharp'." >&2; \
		exit 1; \
	}
	@mkdir -p $(DOCS_API_OUT)/csharp
	$(DOCFX) tools/docs/docfx.json

# Go wrapper documentation. gomarkdoc emits a single markdown file that
# VitePress renders natively (no separate HTML site).
GOMARKDOC ?= gomarkdoc
docs-api-go:
	@echo "=== Generating Go API reference (gomarkdoc) ==="
	@command -v $(GOMARKDOC) >/dev/null 2>&1 || { \
		echo "gomarkdoc not found in PATH. 'go install github.com/princjef/gomarkdoc/cmd/gomarkdoc@latest' or use 'make docker-docs-api-go'." >&2; \
		exit 1; \
	}
	@mkdir -p $(DOCS_SITE_DIR)/content/api-ref
	$(GOMARKDOC) --output $(DOCS_SITE_DIR)/content/api-ref/go.md ./$(SRC_DIR)/wrappers/go/itscam

docs-api: docs-api-clean docs-api-cpp docs-api-python docs-api-csharp docs-api-go
	@echo "=== API reference generated under $(DOCS_API_OUT)/ ==="

docs-api-clean:
	@rm -rf $(DOCS_API_OUT)
	@rm -f $(DOCS_SITE_DIR)/content/api-ref/go.md

docs-sync:
	@echo "=== Syncing VitePress content from README + docs/ ==="
	cd $(DOCS_SITE_DIR) && $(NODE) scripts/sync-content.mjs

# Smoke test for the README -> docs-site mirror. The synced content/
# folder is gitignored, so we cannot diff against a committed copy;
# instead, the check runs sync-content and asserts that the home
# routes (PT-BR + EN) carry recognisable content from the README. This
# catches breakage in the link rewriter or sync script before it ships.
docs-sync-check: docs-sync
	@echo "=== Verifying VitePress home pages mirror README content ==="
	@for f in $(DOCS_SITE_DIR)/content/index.md $(DOCS_SITE_DIR)/content/README.en-US.md; do \
		if [ ! -s "$$f" ]; then \
			echo "Sync check failed: $$f missing or empty." >&2; \
			exit 1; \
		fi; \
	done
	@grep -q "ITSCAM Client SDK" $(DOCS_SITE_DIR)/content/index.md || { \
		echo "Sync check failed: PT-BR home page does not contain 'ITSCAM Client SDK'." >&2; \
		exit 1; \
	}
	@grep -q "ITSCAM Client SDK" $(DOCS_SITE_DIR)/content/README.en-US.md || { \
		echo "Sync check failed: EN home page does not contain 'ITSCAM Client SDK'." >&2; \
		exit 1; \
	}
	@echo "Sync check OK."

docs-site: docs-api
	@echo "=== Building VitePress site ==="
	cd $(DOCS_SITE_DIR) && $(NPM) install --no-audit --no-fund && $(NPM) run build

docker-docs-api-cpp: docker-build
	@echo "=== Generating C/C++ API reference inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make docs-api-cpp

docker-docs-api-python: docker-build
	@echo "=== Generating Python API reference inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make docs-api-python

docker-docs-api-csharp: docker-build
	@echo "=== Generating C# API reference inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make docs-api-csharp

docker-docs-api-go: docker-build
	@echo "=== Generating Go API reference inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make docs-api-go

docker-docs-api: docker-build
	@echo "=== Generating all API references inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make docs-api

docker-docs-site: docker-build
	@echo "=== Building docs site inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make docs-site

# ============================================================================
#  Docker Build
# ============================================================================

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
	$(DOCKER_RUN_IT) $(DOCKER_IMAGE) bash

docker-go-gui: docker-build
	@echo "=== Building Go GUI (Wails) inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make go-gui

docker-go-gui-windows: docker-build
	@echo "=== Building Go GUI for Windows inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make go-gui-windows

docker-csharp: docker-build
	@echo "=== Building C# wrapper inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make csharp

docker-csharp-examples: docker-build
	@echo "=== Building C# wrapper + all examples inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make csharp-examples

docker-csharp-examples-publish: docker-build
	@echo "=== Publishing C# examples (self-contained) inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make csharp-examples-publish

# SPEC=... is forwarded to the container; OUT_DIR=... is forwarded too, but
# remember the path must be valid inside the container (e.g. under /sdk).
docker-codegen: docker-build
	@echo "=== Regenerating typed REST helpers inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make codegen $(if $(SPEC),SPEC=$(SPEC),) $(if $(OUT_DIR),OUT_DIR=$(OUT_DIR),)

docker-codegen-check: docker-build
	@echo "=== Verifying generated REST helpers (in Docker) ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make codegen-check $(if $(SPEC),SPEC=$(SPEC),) $(if $(OUT_DIR),OUT_DIR=$(OUT_DIR),)

# ============================================================================
#  All Platforms
# ============================================================================

all: linux windows examples wrappers
	@echo "=== Build complete ==="

# ============================================================================
#  SDK distribution archive (consumer tar.gz, linux-x64 + win-x64 + win-x86)
# ============================================================================

SDK_DIST_SCRIPT := $(CURDIR)/tools/packaging/make-sdk-dist.sh

sdk-dist: version lib windows csharp-pack
	@echo "=== Packaging SDK distribution ($(SDK_VERSION), linux-x64 + win-x64 + win-x86) ==="
	@$(SDK_DIST_SCRIPT)

sdk-dist-clean:
	@echo "=== Removing SDK distribution artefacts ==="
	@rm -rf dist/

docker-sdk-dist: docker-build
	@echo "=== Packaging SDK distribution inside Docker ==="
	$(DOCKER_RUN) $(DOCKER_IMAGE) make sdk-dist

# ============================================================================
#  Installation
# ============================================================================

install:
	$(MAKE) -C $(SRC_DIR)/core install

# ============================================================================
#  Cleanup
# ============================================================================

clean:
	@echo "=== Cleaning all build artifacts ==="
	$(MAKE) -C $(SRC_DIR)/core clean
	$(MAKE) -C $(SRC_DIR)/examples clean
	@rm -f $(SRC_DIR)/wrappers/go/examples/capture_example \
	       $(SRC_DIR)/wrappers/go/examples/rest_example \
	       $(SRC_DIR)/wrappers/go/examples/cgi_snapshot_example
	@rm -rf $(SRC_DIR)/wrappers/go/examples/gui/build
	@rm -rf $(SRC_DIR)/wrappers/csharp/Itscam.Sdk/bin \
	        $(SRC_DIR)/wrappers/csharp/Itscam.Sdk/obj
	@rm -rf $(SRC_DIR)/wrappers/csharp/examples/CaptureExample/bin \
	        $(SRC_DIR)/wrappers/csharp/examples/CaptureExample/obj
	@rm -rf $(SRC_DIR)/wrappers/csharp/examples/MjpegGrabberExample/bin \
	        $(SRC_DIR)/wrappers/csharp/examples/MjpegGrabberExample/obj
	@rm -rf $(SRC_DIR)/wrappers/csharp/examples/SoftwareTriggerSnapshotExample/bin \
	        $(SRC_DIR)/wrappers/csharp/examples/SoftwareTriggerSnapshotExample/obj
	@rm -rf $(SRC_DIR)/wrappers/csharp/examples/BinaryCaptureExample/bin \
	        $(SRC_DIR)/wrappers/csharp/examples/BinaryCaptureExample/obj
	@rm -rf $(SRC_DIR)/wrappers/csharp/nupkg
	@rm -rf $(CODEGEN_DIR)/build
	@rm -rf dist/
	@rm -rf $(DOCS_API_OUT) $(DOCS_SITE_DIR)/content $(DOCS_SITE_DIR)/.vitepress/dist tools/docs/obj
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
	@echo "  python-example  Show Python binary client example usage"
	@echo "  python-rest-example  Show Python REST example usage"
	@echo "  python-cgi-example   Show Python CGI snapshot example usage"
	@echo "  go-example      Build Go binary-client example"
	@echo "  go-rest-example Build Go REST example"
	@echo "  go-cgi-example  Build Go CGI snapshot example"
	@echo "  go-gui          Build Go GUI example (Wails) for Linux"
	@echo "  go-gui-windows  Build Go GUI example for Windows (cross-compile)"
	@echo "  csharp          Build .NET wrapper (Release)"
	@echo "  csharp-examples Build .NET wrapper + all C# examples"
	@echo "  csharp-examples-publish  Self-contained single-file publish (no runtime needed)"
	@echo "  csharp-pack     Build native artifacts and produce a NuGet"
	@echo "  csharp-pack-linux  NuGet with Linux native binary only (sdk-dist)"
	@echo "  csharp-mjpeg-grabber-example     Build + print run instructions"
	@echo "  csharp-software-trigger-example  Build + print run instructions"
	@echo ""
	@echo "Regression targets (live camera required):"
	@echo "  regression-examples        Build + run all non-GUI examples"
	@echo "  docker-regression-examples Same, inside Docker (--network host)"
	@echo "    Usage: make regression-examples CAMERA_IP=<ip> [CAMERA_USER=admin CAMERA_PASS=secret]"
	@echo "    Artifacts: .regression/<timestamp>_<camera_ip>/ (gitignored)"
	@echo ""
	@echo "Docker targets:"
	@echo "  docker-build    Build the Docker image"
	@echo "  docker-all      Build everything inside Docker"
	@echo "  docker-linux    Build Linux library inside Docker"
	@echo "  docker-windows  Cross-compile for Windows inside Docker"
	@echo "  docker-csharp                   Build C# wrapper inside Docker"
	@echo "  docker-csharp-examples          Build C# wrapper + examples inside Docker"
	@echo "  docker-csharp-examples-publish  Publish self-contained C# binaries inside Docker"
	@echo "  docker-go-gui   Build Go GUI inside Docker (Linux)"
	@echo "  docker-go-gui-windows  Build Go GUI for Windows inside Docker"
	@echo "  docker-codegen                  Regenerate REST types inside Docker"
	@echo "  docker-codegen-check            Verify checked-in REST types (Docker)"
	@echo "  docker-shell    Open interactive shell in Docker"
	@echo ""
	@echo "Code generation targets:"
	@echo "  codegen         Regenerate typed REST helpers (Node 18+ required)"
	@echo "  codegen-check   Verify checked-in REST types match the spec (CI gate)"
	@echo "  Override SPEC=/path/to/itscam.yaml to use a custom OpenAPI document"
	@echo "  Override OUT_DIR=... to write generated files to a different tree"
	@echo ""
	@echo "Documentation targets:"
	@echo "  docs-sync           Run README -> docs-site mirror sync only"
	@echo "  docs-sync-check     CI guard: sync + verify mirror integrity"
	@echo "  docs-api-cpp        Generate Doxygen reference (C/C++)"
	@echo "  docs-api-python     Generate pdoc reference (Python wrapper)"
	@echo "  docs-api-csharp     Generate DocFX reference (C# wrapper)"
	@echo "  docs-api-go         Generate gomarkdoc reference (Go wrapper)"
	@echo "  docs-api            All language API references at once"
	@echo "  docs-site           Sync + docs-api + VitePress production build"
	@echo "  docker-docs-api[-LANG]   Same docs-api targets inside Docker"
	@echo "  docker-docs-site         docs-site inside Docker"
	@echo ""
	@echo "Other targets:"
	@echo "  version         Regenerate version metadata from git tag / commit / date"
	@echo "  sdk-dist        Build consumer tar.gz (linux-x64 + win-x64 + win-x86)"
	@echo "  docker-sdk-dist Same, inside Docker"
	@echo "  sdk-dist-clean  Remove dist/ archives and staging"
	@echo "  all             Build everything"
	@echo "  install         Install library to system"
	@echo "  clean           Remove all build artifacts"
	@echo "  help            Show this help"
	@echo ""
	@echo "Directory structure:"
	@echo "  src/core/       C/C++ SDK library source"
	@echo "  src/examples/   C++ example applications"
	@echo "  src/wrappers/   Language wrappers (python, go, csharp)"
	@echo "  docs/           Chapter-style documentation"

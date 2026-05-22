# ITSCAM SDK Core Library Makefile
#
# Builds the core C/C++ shared library with cross-platform support.
#
# Targets:
#   make linux          - Build for Linux (native)
#   make windows        - Cross-compile for Windows (MinGW)
#   make all            - Build for all platforms
#   make clean          - Clean build artifacts
#
# Copyright (c) 2026 Pumatronix

.PHONY: all linux windows clean install help

# Default target
all: linux

# Build directories
BUILD_DIR        := build
BUILD_DIR_LINUX  := $(BUILD_DIR)/linux
BUILD_DIR_WINDOWS := $(BUILD_DIR)/windows

# Third-party dependencies (committed in repository)
3RDPARTY_DIR     := 3rdparty

# Compilers
CXX_LINUX   := g++
# Use win32 variant - we have our own OS abstraction layer for threading
CXX_WINDOWS := x86_64-w64-mingw32-g++

# Common flags
CXXFLAGS_COMMON := -std=c++14 -O2 -fPIC -Wall -Wextra

# Platform-specific flags
# Note: Using default visibility for C++ API compatibility
CXXFLAGS_LINUX   := $(CXXFLAGS_COMMON)
# ITSCAM_SDK_BUILDING enables dllexport in the C API header
CXXFLAGS_WINDOWS := $(CXXFLAGS_COMMON) -DWIN32 -D_WIN32 -DITSCAM_SDK_BUILDING

# Linker flags
LDFLAGS_LINUX   := -shared -lpthread
# Static link libgcc and libstdc++ to avoid DLL dependencies
# Uses Win32 native threading (no pthread dependency)
LDFLAGS_WINDOWS := -shared -lws2_32 -static-libgcc -static-libstdc++

# Include paths
INCLUDES := -I. -Ic_api -I$(3RDPARTY_DIR)

# Source files
SOURCES_COMMON := itscam_client.cpp c_api/itscam_sdk_c.cpp
SOURCE_LINUX   := itscam_rest_client.cpp impl/itscam_sdk_utils_linux.cpp impl/itscam_os_linux.cpp
# Note: REST client excluded from Windows build (uses httplib which requires pthread)
# To enable REST client on Windows, use the mingw-posix variant compiler
SOURCE_WINDOWS := impl/itscam_sdk_utils_windows.cpp impl/itscam_os_windows.cpp

# Output files
LIB_LINUX_SO     := $(BUILD_DIR_LINUX)/libitscam_sdk.so
LIB_LINUX_SONAME := libitscam_sdk.so.1
LIB_LINUX_REAL   := libitscam_sdk.so.1.0.0
LIB_LINUX_A      := $(BUILD_DIR_LINUX)/libitscam_sdk.a
LIB_WINDOWS      := $(BUILD_DIR_WINDOWS)/itscam_sdk.dll
IMPLIB_WINDOWS   := $(BUILD_DIR_WINDOWS)/libitscam_sdk.a
LIB_WINDOWS_A    := $(BUILD_DIR_WINDOWS)/libitscam_sdk_static.a

# Object files
OBJS_COMMON_LINUX  := $(patsubst %.cpp,$(BUILD_DIR_LINUX)/%.o,$(SOURCES_COMMON))
OBJS_LINUX         := $(OBJS_COMMON_LINUX) $(BUILD_DIR_LINUX)/itscam_rest_client.o $(BUILD_DIR_LINUX)/impl/itscam_sdk_utils_linux.o $(BUILD_DIR_LINUX)/impl/itscam_os_linux.o
OBJS_COMMON_WIN    := $(patsubst %.cpp,$(BUILD_DIR_WINDOWS)/%.o,$(SOURCES_COMMON))
OBJS_WINDOWS       := $(OBJS_COMMON_WIN) $(BUILD_DIR_WINDOWS)/impl/itscam_sdk_utils_windows.o $(BUILD_DIR_WINDOWS)/impl/itscam_os_windows.o

# ============================================================================
#  Linux build
# ============================================================================

linux: $(LIB_LINUX_SO) $(LIB_LINUX_A)

$(LIB_LINUX_SO): $(OBJS_LINUX)
	@echo "Linking libitscam_sdk.so..."
	@$(CXX_LINUX) $(LDFLAGS_LINUX) -Wl,-soname,$(LIB_LINUX_SONAME) -o $(BUILD_DIR_LINUX)/$(LIB_LINUX_REAL) $^
	@ln -sf $(LIB_LINUX_REAL) $(BUILD_DIR_LINUX)/$(LIB_LINUX_SONAME)
	@ln -sf $(LIB_LINUX_REAL) $(LIB_LINUX_SO)

$(LIB_LINUX_A): $(OBJS_LINUX)
	@echo "Creating libitscam_sdk.a..."
	@ar rcs $@ $^

$(BUILD_DIR_LINUX)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling (Linux): $<"
	@$(CXX_LINUX) $(CXXFLAGS_LINUX) $(INCLUDES) -c $< -o $@

# ============================================================================
#  Windows build (cross-compile)
# ============================================================================

windows: check-mingw $(LIB_WINDOWS) $(LIB_WINDOWS_A)

$(LIB_WINDOWS): $(OBJS_WINDOWS)
	@echo "Linking itscam_sdk.dll..."
	@$(CXX_WINDOWS) -o $@ $^ $(LDFLAGS_WINDOWS) -Wl,--out-implib,$(IMPLIB_WINDOWS)

$(LIB_WINDOWS_A): $(OBJS_WINDOWS)
	@echo "Creating libitscam_sdk_static.a (Windows)..."
	@x86_64-w64-mingw32-ar rcs $@ $^

$(BUILD_DIR_WINDOWS)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling (Windows): $<"
	@$(CXX_WINDOWS) $(CXXFLAGS_WINDOWS) $(INCLUDES) -c $< -o $@

check-mingw:
	@which $(CXX_WINDOWS) > /dev/null || (echo "Error: MinGW not found. Install mingw-w64." && exit 1)

# ============================================================================
#  Installation
# ============================================================================

PREFIX ?= /usr/local

install: linux
	@echo "Installing to $(PREFIX)..."
	@install -d $(PREFIX)/lib
	@install -d $(PREFIX)/include/itscam
	@install -m 644 $(BUILD_DIR_LINUX)/$(LIB_LINUX_REAL) $(PREFIX)/lib/
	@ln -sf $(LIB_LINUX_REAL) $(PREFIX)/lib/$(LIB_LINUX_SONAME)
	@ln -sf $(LIB_LINUX_REAL) $(PREFIX)/lib/libitscam_sdk.so
	@install -m 644 itscam_client.h $(PREFIX)/include/itscam/
	@install -m 644 itscam_rest_client.h $(PREFIX)/include/itscam/
	@install -m 644 itscam_sdk.h $(PREFIX)/include/itscam/
	@install -m 644 itscam_sdk_c.h $(PREFIX)/include/itscam/
	@install -m 644 itscam_sdk_utils.h $(PREFIX)/include/itscam/
	@install -m 644 itscam_types.h $(PREFIX)/include/itscam/
	@install -m 644 itscam_jpeg_utils.h $(PREFIX)/include/itscam/
	@ldconfig 2>/dev/null || true
	@echo "Installation complete."

# ============================================================================
#  Cleanup
# ============================================================================

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)

# ============================================================================
#  Help
# ============================================================================

help:
	@echo "ITSCAM SDK Core Library"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  linux         Build for Linux (native) - default"
	@echo "  windows       Cross-compile for Windows (MinGW)"
	@echo "  all           Build for all platforms"
	@echo "  deps          Download dependencies only"
	@echo "  clean         Remove build artifacts"
	@echo "  install       Install to system (Linux)"
	@echo "  help          Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX        Installation prefix (default: /usr/local)"

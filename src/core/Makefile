# ITSCAM SDK Core Library Makefile
#
# Builds the core C/C++ shared library with cross-platform support and
# statically-linked mbedTLS for HTTPS.
#
# Targets:
#   make linux          - Build for Linux (native)
#   make windows        - Cross-compile for Windows (MinGW)
#   make all            - Build for all platforms
#   make clean          - Clean build artifacts
#
# Copyright (c) 2026 Pumatronix

.PHONY: all linux windows windows-x64 windows-x86 clean install help version

# Default target
all: linux

REPO_ROOT     := $(abspath $(CURDIR)/../..)
VERSION_SCRIPT := $(REPO_ROOT)/tools/version/gen-version.sh
-include $(REPO_ROOT)/tools/version/sdk-version.mk
SDK_LIB_VERSION ?= 0.0.0

version:
	@$(VERSION_SCRIPT)

# Build directories
BUILD_DIR        := build
BUILD_DIR_LINUX  := $(BUILD_DIR)/linux
BUILD_DIR_WIN_X64 := $(BUILD_DIR)/win-x64
BUILD_DIR_WIN_X86 := $(BUILD_DIR)/win-x86

# Third-party dependencies (committed in repository)
3RDPARTY_DIR     := 3rdparty
MBEDTLS_DIR      := $(3RDPARTY_DIR)/mbedtls
MBEDTLS_INC      := $(MBEDTLS_DIR)/include

# Compilers
CC_LINUX    := gcc
CXX_LINUX   := g++
# MinGW win32 threading model (not posix).  libgcc/libstdc++ are linked
# statically so consumers do not need libgcc.dll / libstdc++-6.dll.
CC_WIN_X64  := x86_64-w64-mingw32-gcc
CXX_WIN_X64 := x86_64-w64-mingw32-g++
AR_WIN_X64  := x86_64-w64-mingw32-ar

CC_WIN_X86  := i686-w64-mingw32-gcc
CXX_WIN_X86 := i686-w64-mingw32-g++
AR_WIN_X86  := i686-w64-mingw32-ar

# Common flags
CXXFLAGS_COMMON := -std=c++17 -O2 -fPIC -Wall -Wextra

# CPPHTTPLIB_MBEDTLS_SUPPORT lights up the SSLClient path inside httplib.h.
# mbedTLS sources are pre-fetched and committed under 3rdparty/mbedtls/,
# so this is normally always enabled.  We still guard on the presence of
# the headers so out-of-tree consumers that prune the vendored sources
# still get a working (HTTP-only) build.
ifneq (,$(wildcard $(MBEDTLS_INC)/mbedtls/ssl.h))
    TLS_DEFINES := -DCPPHTTPLIB_MBEDTLS_SUPPORT
else
    TLS_DEFINES :=
endif

# Platform-specific flags
CXXFLAGS_LINUX   := $(CXXFLAGS_COMMON) $(TLS_DEFINES)
# ITSCAM_SDK_BUILDING enables dllexport in the C API header
# ITSCAM_MINGW_WIN32_THREADS: use Win32 sync shims instead of libstdc++ <thread>
CXXFLAGS_WINDOWS := $(CXXFLAGS_COMMON) $(TLS_DEFINES) -DWIN32 -D_WIN32 \
    -DITSCAM_SDK_BUILDING -DITSCAM_MINGW_WIN32_THREADS

# mbedTLS C compile flags.  The custom user config lives next to the
# vendored sources, so we add $(MBEDTLS_DIR) to the include path.
MBEDTLS_CFLAGS_COMMON := -O2 -fvisibility=hidden \
    -DMBEDTLS_USER_CONFIG_FILE='"itscam_mbedtls_config.h"' \
    -I$(MBEDTLS_INC) -I$(MBEDTLS_DIR)
MBEDTLS_CFLAGS_LINUX   := $(MBEDTLS_CFLAGS_COMMON) -fPIC
MBEDTLS_CFLAGS_WINDOWS := $(MBEDTLS_CFLAGS_COMMON)

# Linker flags
LDFLAGS_LINUX   := -shared -lpthread
# bcrypt is required by mbedTLS' Windows RNG.  Static link libgcc and
# libstdc++ to avoid DLL dependencies; native Win32 threading is used.
LDFLAGS_WINDOWS := -shared -lws2_32 -lbcrypt -lcrypt32 -static-libgcc -static-libstdc++

# Include paths
INCLUDES := -I. -Ic_api -I$(3RDPARTY_DIR) -I$(MBEDTLS_INC)

# ============================================================================
#  Source files
# ============================================================================

SOURCES_COMMON := \
    itscam_client.cpp \
    c_api/itscam_sdk_c.cpp \
    c_api/itscam_rest_client_c.cpp \
    c_api/itscam_cgi_client_c.cpp

SOURCES_NETWORK := \
    itscam_rest_client.cpp \
    itscam_cgi_client.cpp \
    impl/itscam_http_transport.cpp \
    impl/itscam_multipart.cpp

SOURCE_LINUX_PLATFORM := \
    impl/itscam_sdk_utils_linux.cpp \
    impl/itscam_os_linux.cpp

SOURCE_WINDOWS_PLATFORM := \
    impl/itscam_sdk_utils_windows.cpp \
    impl/itscam_os_windows.cpp

SDK_SOURCES_LINUX   := $(SOURCES_COMMON) $(SOURCES_NETWORK) $(SOURCE_LINUX_PLATFORM)
SDK_SOURCES_WINDOWS := $(SOURCES_COMMON) $(SOURCES_NETWORK) $(SOURCE_WINDOWS_PLATFORM)

# mbedTLS sources are pre-fetched and committed under 3rdparty/mbedtls/.
# If the directory has been pruned, this expands to an empty list and the
# SDK is built without HTTPS support.
MBEDTLS_SRCS := $(wildcard $(MBEDTLS_DIR)/library/*.c)

OBJS_SDK_LINUX     := $(patsubst %.cpp,$(BUILD_DIR_LINUX)/%.o,$(SDK_SOURCES_LINUX))
OBJS_WIN_X64       := $(patsubst %.cpp,$(BUILD_DIR_WIN_X64)/%.o,$(SDK_SOURCES_WINDOWS))
OBJS_WIN_X86       := $(patsubst %.cpp,$(BUILD_DIR_WIN_X86)/%.o,$(SDK_SOURCES_WINDOWS))
OBJS_MBEDTLS_LINUX := $(patsubst $(MBEDTLS_DIR)/library/%.c,$(BUILD_DIR_LINUX)/mbedtls/%.o,$(MBEDTLS_SRCS))
OBJS_MBEDTLS_WIN_X64 := $(patsubst $(MBEDTLS_DIR)/library/%.c,$(BUILD_DIR_WIN_X64)/mbedtls/%.o,$(MBEDTLS_SRCS))
OBJS_MBEDTLS_WIN_X86 := $(patsubst $(MBEDTLS_DIR)/library/%.c,$(BUILD_DIR_WIN_X86)/mbedtls/%.o,$(MBEDTLS_SRCS))

OBJS_LINUX   := $(OBJS_SDK_LINUX)   $(OBJS_MBEDTLS_LINUX)
OBJS_WIN_X64 := $(OBJS_WIN_X64)     $(OBJS_MBEDTLS_WIN_X64)
OBJS_WIN_X86 := $(OBJS_WIN_X86)     $(OBJS_MBEDTLS_WIN_X86)

# Output files
LIB_LINUX_SO     := $(BUILD_DIR_LINUX)/libitscam_sdk.so
LIB_LINUX_SONAME := libitscam_sdk.so.$(word 1,$(subst ., ,$(SDK_LIB_VERSION)))
LIB_LINUX_REAL   := $(BUILD_DIR_LINUX)/libitscam_sdk.so.$(SDK_LIB_VERSION)
LIB_LINUX_A      := $(BUILD_DIR_LINUX)/libitscam_sdk.a

LIB_WIN_X64_DLL    := $(BUILD_DIR_WIN_X64)/itscam_sdk.dll
LIB_WIN_X64_IMPLIB := $(BUILD_DIR_WIN_X64)/libitscam_sdk.a
LIB_WIN_X64_STATIC := $(BUILD_DIR_WIN_X64)/libitscam_sdk_static.a

LIB_WIN_X86_DLL    := $(BUILD_DIR_WIN_X86)/itscam_sdk.dll
LIB_WIN_X86_IMPLIB := $(BUILD_DIR_WIN_X86)/libitscam_sdk.a
LIB_WIN_X86_STATIC := $(BUILD_DIR_WIN_X86)/libitscam_sdk_static.a

# ============================================================================
#  Linux build
# ============================================================================

linux: version $(LIB_LINUX_SO) $(LIB_LINUX_A)

$(LIB_LINUX_SO): $(OBJS_LINUX)
	@echo "Linking libitscam_sdk.so..."
	@$(CXX_LINUX) $(LDFLAGS_LINUX) -Wl,-soname,$(LIB_LINUX_SONAME) -o $(LIB_LINUX_REAL) $^
	@ln -sf libitscam_sdk.so.$(SDK_LIB_VERSION) $(BUILD_DIR_LINUX)/$(LIB_LINUX_SONAME)
	@ln -sf libitscam_sdk.so.$(SDK_LIB_VERSION) $(LIB_LINUX_SO)

$(LIB_LINUX_A): $(OBJS_LINUX)
	@echo "Creating libitscam_sdk.a..."
	@ar rcs $@ $^

$(BUILD_DIR_LINUX)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling (Linux): $<"
	@$(CXX_LINUX) $(CXXFLAGS_LINUX) $(INCLUDES) -c $< -o $@

$(BUILD_DIR_LINUX)/mbedtls/%.o: $(MBEDTLS_DIR)/library/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling (mbedtls/Linux): $<"
	@$(CC_LINUX) $(MBEDTLS_CFLAGS_LINUX) -c $< -o $@

# ============================================================================
#  Windows build (cross-compile, win32 + win64)
# ============================================================================

windows: windows-x64 windows-x86

windows-x64: check-mingw-x64 $(LIB_WIN_X64_DLL) $(LIB_WIN_X64_STATIC)

windows-x86: check-mingw-x86 $(LIB_WIN_X86_DLL) $(LIB_WIN_X86_STATIC)

$(LIB_WIN_X64_DLL): $(OBJS_WIN_X64)
	@echo "Linking win-x64/itscam_sdk.dll..."
	@$(CXX_WIN_X64) -o $@ $^ $(LDFLAGS_WINDOWS) -Wl,--out-implib,$(LIB_WIN_X64_IMPLIB)

$(LIB_WIN_X64_STATIC): $(OBJS_WIN_X64)
	@echo "Creating win-x64/libitscam_sdk_static.a..."
	@$(AR_WIN_X64) rcs $@ $^

$(LIB_WIN_X86_DLL): $(OBJS_WIN_X86)
	@echo "Linking win-x86/itscam_sdk.dll..."
	@$(CXX_WIN_X86) -o $@ $^ $(LDFLAGS_WINDOWS) -Wl,--out-implib,$(LIB_WIN_X86_IMPLIB)

$(LIB_WIN_X86_STATIC): $(OBJS_WIN_X86)
	@echo "Creating win-x86/libitscam_sdk_static.a..."
	@$(AR_WIN_X86) rcs $@ $^

$(BUILD_DIR_WIN_X64)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling (win-x64): $<"
	@$(CXX_WIN_X64) $(CXXFLAGS_WINDOWS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR_WIN_X86)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling (win-x86): $<"
	@$(CXX_WIN_X86) $(CXXFLAGS_WINDOWS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR_WIN_X64)/mbedtls/%.o: $(MBEDTLS_DIR)/library/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling (mbedtls/win-x64): $<"
	@$(CC_WIN_X64) $(MBEDTLS_CFLAGS_WINDOWS) -c $< -o $@

$(BUILD_DIR_WIN_X86)/mbedtls/%.o: $(MBEDTLS_DIR)/library/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling (mbedtls/win-x86): $<"
	@$(CC_WIN_X86) $(MBEDTLS_CFLAGS_WINDOWS) -c $< -o $@

check-mingw-x64:
	@which $(CXX_WIN_X64) > /dev/null || (echo "Error: x86_64 MinGW not found. Install mingw-w64." && exit 1)

check-mingw-x86:
	@which $(CXX_WIN_X86) > /dev/null || (echo "Error: i686 MinGW not found. Install g++-mingw-w64-i686." && exit 1)

# ============================================================================
#  Installation
# ============================================================================

PREFIX ?= /usr/local

install: linux
	@echo "Installing to $(PREFIX)..."
	@install -d $(PREFIX)/lib
	@install -d $(PREFIX)/include/itscam
	@install -m 644 $(LIB_LINUX_REAL) $(PREFIX)/lib/
	@ln -sf libitscam_sdk.so.$(SDK_LIB_VERSION) $(PREFIX)/lib/$(LIB_LINUX_SONAME)
	@ln -sf libitscam_sdk.so.$(SDK_LIB_VERSION) $(PREFIX)/lib/libitscam_sdk.so
	@install -m 644 itscam_client.h $(PREFIX)/include/itscam/
	@install -m 644 itscam_rest_client.h $(PREFIX)/include/itscam/
	@install -m 644 itscam_cgi_client.h $(PREFIX)/include/itscam/
	@install -m 644 itscam_sdk.h $(PREFIX)/include/itscam/
	@install -m 644 itscam_sdk_version.h $(PREFIX)/include/itscam/
	@install -m 644 c_api/itscam_sdk_c.h $(PREFIX)/include/itscam/
	@install -m 644 c_api/itscam_rest_client_c.h $(PREFIX)/include/itscam/
	@install -m 644 c_api/itscam_cgi_client_c.h $(PREFIX)/include/itscam/
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
	@rm -rf $(BUILD_DIR_LINUX) $(BUILD_DIR_WIN_X64) $(BUILD_DIR_WIN_X86)

# ============================================================================
#  Help
# ============================================================================

help:
	@echo "ITSCAM SDK Core Library"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  linux           Build for Linux (native) - default"
	@echo "  windows         Cross-compile for Windows (win-x64 + win-x86)"
	@echo "  windows-x64     Cross-compile for Windows x64"
	@echo "  windows-x86     Cross-compile for Windows x86 (win32)"
	@echo "  all             Build for all platforms"
	@echo "  clean           Remove build artifacts"
	@echo "  install         Install to system (Linux)"
	@echo "  help            Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX          Installation prefix (default: /usr/local)"
	@echo ""
	@echo "Notes:"
	@echo "  HTTPS support is built in - mbedTLS sources are vendored under"
	@echo "  3rdparty/mbedtls/ and statically linked into libitscam_sdk."

# ITSCAM SDK - C++ Examples Makefile
#
# Usage:
#   make                Build all Linux examples (requires lib to be built first)
#   make windows        Cross-compile all Windows examples (win-x64 + win-x86)
#   make clean          Remove build artifacts
#
# The shared library must be built first from the core/ directory or via
# the top-level Makefile.
#
# Copyright (c) 2026 Pumatronix

# -- Paths -------------------------------------------------------------------
CORE_DIR   := ../core
DEPS_DIR   := $(CORE_DIR)/3rdparty
BUILD_DIR  := build
LIB_DIR_LINUX       := $(CORE_DIR)/build/linux
LIB_DIR_WIN_X64     := $(CORE_DIR)/build/win-x64
LIB_DIR_WIN_X86     := $(CORE_DIR)/build/win-x86
LIB_STATIC_WIN_X64  := $(LIB_DIR_WIN_X64)/libitscam_sdk_static.a
LIB_STATIC_WIN_X86  := $(LIB_DIR_WIN_X86)/libitscam_sdk_static.a

BUILD_DIR_WIN_X64 := $(BUILD_DIR)/win-x64
BUILD_DIR_WIN_X86 := $(BUILD_DIR)/win-x86

# -- Toolchain ---------------------------------------------------------------
CXX_LINUX      ?= g++
CXX_WIN_X64    ?= x86_64-w64-mingw32-g++
CXX_WIN_X86    ?= i686-w64-mingw32-g++

CXXFLAGS_LINUX := -std=c++17 -Wall -Wextra
CXXFLAGS_WINDOWS := -std=c++17 -Wall -Wextra -DWIN32 -D_WIN32 -DITSCAM_MINGW_WIN32_THREADS

INCLUDES := -I$(CORE_DIR) -I$(DEPS_DIR)

# -- Examples ----------------------------------------------------------------
LINUX_EXAMPLES := \
	$(BUILD_DIR)/itscam_sdk_example \
	$(BUILD_DIR)/itscam_rest_example \
	$(BUILD_DIR)/itscam_cgi_example \
	$(BUILD_DIR)/itscam_trigger_recorder

WIN_X64_EXAMPLES := \
	$(BUILD_DIR_WIN_X64)/itscam_sdk_example.exe \
	$(BUILD_DIR_WIN_X64)/itscam_rest_example.exe \
	$(BUILD_DIR_WIN_X64)/itscam_cgi_example.exe \
	$(BUILD_DIR_WIN_X64)/itscam_trigger_recorder.exe

WIN_X86_EXAMPLES := \
	$(BUILD_DIR_WIN_X86)/itscam_sdk_example.exe \
	$(BUILD_DIR_WIN_X86)/itscam_rest_example.exe \
	$(BUILD_DIR_WIN_X86)/itscam_cgi_example.exe \
	$(BUILD_DIR_WIN_X86)/itscam_trigger_recorder.exe

EXAMPLE_SOURCES := \
	itscam_sdk_example.cpp \
	itscam_rest_example.cpp \
	itscam_cgi_example.cpp \
	itscam_trigger_recorder.cpp

.PHONY: all windows windows-x64 windows-x86 clean help

all: $(LINUX_EXAMPLES)

windows: windows-x64 windows-x86

windows-x64: $(WIN_X64_EXAMPLES)

windows-x86: $(WIN_X86_EXAMPLES)

# ============================================================================
#  Build directories
# ============================================================================

$(BUILD_DIR) $(BUILD_DIR_WIN_X64) $(BUILD_DIR_WIN_X86):
	@mkdir -p $@

# ============================================================================
#  Linux examples (linked against shared library with rpath)
# ============================================================================

define linux_example_rule
$(BUILD_DIR)/$(1): $(1).cpp | $(BUILD_DIR)
	@echo "Building $(1)..."
	$(CXX_LINUX) $(CXXFLAGS_LINUX) $(INCLUDES) $$< -o $$@ \
	    -L$(LIB_DIR_LINUX) -litscam_sdk -lpthread \
	    -Wl,-rpath,'$$$$ORIGIN:$(LIB_DIR_LINUX)'
endef

$(foreach name,itscam_sdk_example itscam_rest_example itscam_cgi_example itscam_trigger_recorder,$(eval $(call linux_example_rule,$(name))))

# ============================================================================
#  Windows examples (MinGW cross-compile; ship itscam_sdk.dll beside .exe)
# ============================================================================

define windows_example_rule
$(BUILD_DIR_$(2))/$(1).exe: $(1).cpp | $(BUILD_DIR_$(2))
	@echo "Building $(1).exe ($(3), static)..."
	$(CXX_$(2)) $(CXXFLAGS_WINDOWS) $(INCLUDES) $$< -o $$@ \
	    $(LIB_STATIC_$(2)) \
	    -static-libgcc -static-libstdc++ -lws2_32 -lbcrypt -lcrypt32
endef

$(foreach name,itscam_sdk_example itscam_rest_example itscam_cgi_example itscam_trigger_recorder,$(eval $(call windows_example_rule,$(name),WIN_X64,win-x64)))
$(foreach name,itscam_sdk_example itscam_rest_example itscam_cgi_example itscam_trigger_recorder,$(eval $(call windows_example_rule,$(name),WIN_X86,win-x86)))

# ============================================================================
#  Cleanup
# ============================================================================

clean:
	rm -rf $(BUILD_DIR)

# ============================================================================
#  Help
# ============================================================================

help:
	@echo "ITSCAM SDK C++ Examples"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "NOTE: Build the core library first (from parent directory or core/)"
	@echo ""
	@echo "Targets:"
	@echo "  all           Build all Linux examples (default)"
	@echo "  windows       Cross-compile all Windows examples (win-x64 + win-x86)"
	@echo "  windows-x64   Cross-compile 64-bit Windows examples"
	@echo "  windows-x86   Cross-compile 32-bit Windows examples"
	@echo "  clean         Remove build artifacts"
	@echo "  help          Show this help"

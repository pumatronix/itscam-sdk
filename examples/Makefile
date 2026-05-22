# ITSCAM SDK - C++ Examples Makefile
#
# Usage:
#   make                Build all examples (requires lib to be built first)
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
LIB_DIR    := $(CORE_DIR)/build/linux

# -- Toolchain ---------------------------------------------------------------
CXX      ?= g++
CXXFLAGS := -std=c++14 -Wall -Wextra
LDFLAGS  := -lpthread

INCLUDES := -I$(CORE_DIR) -I$(DEPS_DIR)

# -- Examples ----------------------------------------------------------------
EXAMPLES := \
	$(BUILD_DIR)/itscam_sdk_example \
	$(BUILD_DIR)/itscam_rest_example \
	$(BUILD_DIR)/itscam_trigger_recorder

.PHONY: all clean help

all: $(EXAMPLES)

# ============================================================================
#  Build directory
# ============================================================================

$(BUILD_DIR):
	@mkdir -p $@

# ============================================================================
#  Examples (linked against shared library with rpath)
# ============================================================================

$(BUILD_DIR)/itscam_sdk_example: itscam_sdk_example.cpp | $(BUILD_DIR)
	@echo "Building itscam_sdk_example..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ -L$(LIB_DIR) -litscam_sdk -lpthread -Wl,-rpath,'$$ORIGIN:$(LIB_DIR)'

$(BUILD_DIR)/itscam_rest_example: itscam_rest_example.cpp | $(BUILD_DIR)
	@echo "Building itscam_rest_example..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ -L$(LIB_DIR) -litscam_sdk -lpthread -Wl,-rpath,'$$ORIGIN:$(LIB_DIR)'

$(BUILD_DIR)/itscam_trigger_recorder: itscam_trigger_recorder.cpp | $(BUILD_DIR)
	@echo "Building itscam_trigger_recorder..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ -L$(LIB_DIR) -litscam_sdk -lpthread -Wl,-rpath,'$$ORIGIN:$(LIB_DIR)'

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
	@echo "  all           Build all examples (default)"
	@echo "  clean         Remove build artifacts"
	@echo "  help          Show this help"


#!/bin/bash
# build.sh - Helper script for building, uploading, and monitoring firmware
# Usage: ./build.sh <target>
# Example: ./build.sh hamster

set -e


CMD="$1"
TARGET="$2"




# List of supported projects and platforms (update as needed)
PROJECTS=(hamster sunflower raspi nrf_bench)
PLATFORMS=(avr-168 avr-328p raspi)




function usage() {
    echo "Usage: $0 <project> [platform] | clean"
    echo "Available projects: ${PROJECTS[*]}"
    echo "Available platforms: ${PLATFORMS[*]} (default: avr-168, raspi for raspi project)"
    echo "Note: raspi builds only the CMake target 'raspi' and skips upload/monitor."
    exit 1
}





# Clean command
if [[ "$CMD" == "clean" ]]; then
    echo "Cleaning all targets..."
    for plat in "${PLATFORMS[@]}"; do
        cmake --build --preset $plat --target clean 2>/dev/null || true
    done
    exit 0
fi

# Default project/platform
PROJECT="$CMD"
PLATFORM="$TARGET"
if [[ -z "$PROJECT" ]]; then
    usage
fi
if [[ -z "$PLATFORM" ]]; then
    if [[ "$PROJECT" == "raspi" ]]; then
        PLATFORM="raspi"
    else
        PLATFORM="avr-168"
    fi
fi

# Validate project
if [[ ! " ${PROJECTS[@]} " =~ " $PROJECT " ]]; then
    echo "Unknown project: $PROJECT"
    usage
fi
# Validate platform
if [[ ! " ${PLATFORMS[@]} " =~ " $PLATFORM " ]]; then
    echo "Unknown platform: $PLATFORM"
    usage
fi

# Keep project/platform combinations explicit to avoid confusing builds.
if [[ "$PROJECT" == "raspi" && "$PLATFORM" != "raspi" ]]; then
    echo "Project 'raspi' must use platform 'raspi'."
    usage
fi
if [[ "$PROJECT" != "raspi" && "$PLATFORM" == "raspi" ]]; then
    echo "Platform 'raspi' is only valid for project 'raspi'."
    usage
fi

if [[ "$PROJECT" == "raspi" ]]; then
    echo "Configuring build directory for $PLATFORM..."
    cmake --preset $PLATFORM

    RASPI_CACHE_FILE="build-$PLATFORM/CMakeCache.txt"
    RASPI_RUST_TARGET=""
    if [[ -f "$RASPI_CACHE_FILE" ]]; then
        RASPI_RUST_TARGET="$(sed -n 's/^RASPI_RUST_TARGET:STRING=//p' "$RASPI_CACHE_FILE" | head -n1)"
    fi

    if [[ -n "$RASPI_RUST_TARGET" ]]; then
        if command -v rustup >/dev/null 2>&1; then
            echo "Ensuring Rust target is installed: $RASPI_RUST_TARGET"
            rustup target add "$RASPI_RUST_TARGET"
        else
            echo "rustup not found. Install rustup and run: rustup target add $RASPI_RUST_TARGET"
            exit 1
        fi
    fi

    echo "Building Raspberry Pi binary (ARMv6) via Cargo target from CMake..."
    cmake --build --preset $PLATFORM --target raspi
    exit 0
fi

# Ensure build directory exists for the platform
BUILD_DIR="build-$PLATFORM"
if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Configuring build directory for $PLATFORM..."
    cmake --preset $PLATFORM
fi

# Build firmware for project
cmake --build --preset $PLATFORM --target firmware_$PROJECT

# Upload firmware for project
cmake --build --preset $PLATFORM --target upload_$PROJECT

# Start monitor for project
cmake --build --preset $PLATFORM --target monitor_$PROJECT

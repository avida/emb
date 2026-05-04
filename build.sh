#!/bin/bash
# build.sh - Helper script for building, uploading, and monitoring firmware
# Usage: ./build.sh <target>
# Example: ./build.sh hamster

set -e


CMD="$1"
TARGET="$2"

# List of supported targets (update as needed)
TARGETS=(hamster sunflower)

function usage() {
    echo "Usage: $0 <target> | clean"
    echo "Available targets: ${TARGETS[*]}"
    exit 1
}

BUILD_DIR="build-avr-168"

if [[ -z "$CMD" ]]; then
    usage
fi

if [[ "$CMD" == "clean" ]]; then
    echo "Cleaning all targets..."
    cmake --build "$BUILD_DIR" --target clean
    exit 0
fi

TARGET="$CMD"
if [[ ! " ${TARGETS[@]} " =~ " $TARGET " ]]; then
    echo "Unknown target: $TARGET"
    usage
fi

# Build firmware
cmake --build "$BUILD_DIR" --target firmware_$TARGET

# Upload firmware
cmake --build "$BUILD_DIR" --target upload_$TARGET

# Start monitor
cmake --build "$BUILD_DIR" --target monitor_$TARGET

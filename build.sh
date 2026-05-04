#!/bin/bash
# build.sh - Helper script for building, uploading, and monitoring firmware
# Usage: ./build.sh <target>
# Example: ./build.sh hamster

set -e


CMD="$1"
TARGET="$2"




# List of supported projects and platforms (update as needed)
PROJECTS=(hamster sunflower)
PLATFORMS=(avr-168 avr-328p)




function usage() {
    echo "Usage: $0 <project> [platform] | clean"
    echo "Available projects: ${PROJECTS[*]}"
    echo "Available platforms: ${PLATFORMS[*]} (default: avr-168)"
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
    PLATFORM="avr-168"
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


if [[ "$CMD" == "clean" ]]; then
    echo "Cleaning all targets..."
    cmake --build --preset avr-168 --target clean
    cmake --build --preset avr-328p --target clean 2>/dev/null || true
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

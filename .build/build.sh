#!/bin/bash
set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/.build"
OUT="$BUILD_DIR/server_monitor"

echo "Building server-monitor..."
echo "====================================================="

mkdir -p "$BUILD_DIR"

echo
echo "Cleaning previous builds..."
rm -f "$BUILD_DIR/server_monitor"

echo
echo "Collecting .cpp files..."
CPP_FILES=$(find "$ROOT_DIR" -name "*.cpp" -not -path "$BUILD_DIR/*" | sort)
echo "$CPP_FILES" | sed 's/^/  /'

if [ -z "$CPP_FILES" ]; then
    echo "ERROR: No .cpp files found."
    exit 1
fi

echo
echo "Compiling..."
g++ -std=c++17 -O2 \
    -I"$ROOT_DIR" \
    -o "$OUT" \
    $CPP_FILES \
    -lpthread

echo
if [ -f "$OUT" ]; then
    echo "BUILD SUCCESSFUL!"
    echo "Created: $OUT"
else
    echo "ERROR: Output not created."
    exit 1
fi
cat > .build/build.sh << 'EOF'
#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$SCRIPT_DIR"
OUT="$BUILD_DIR/server_monitor"

echo "Building server-monitor..."
echo "====================================================="

echo
echo "Cleaning previous builds..."
rm -f "$OUT"

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
EOF
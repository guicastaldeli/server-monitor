#!/bin/bash
set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$ROOT_DIR/.build/server_monitor"

if [ ! -x "$BIN" ]; then
    echo "ERROR: server_monitor not found. Run ./build.sh first."
    exit 1
fi

"$BIN" "$@"
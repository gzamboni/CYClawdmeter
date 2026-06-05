#!/bin/bash
# Build and flash Clawdmeter firmware on macOS (CYD / ESP32-2432S028R).
# Usage:
#   ./flash-mac.sh                          # auto-detect /dev/cu.wchusbserial*
#   ./flash-mac.sh /dev/cu.wchusbserial14210  # explicit USB serial port
#
# CYD uses CH340 USB-serial. Enter flash mode: hold BOOT, press+release RST, release BOOT.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BOARD="esp32_2432s028r"
PORT="$1"

if [ -z "$PORT" ]; then
    PORT=$(ls /dev/cu.wchusbserial* 2>/dev/null | head -1)
    if [ -z "$PORT" ]; then
        echo "Error: no /dev/cu.wchusbserial* device found. Plug in via USB."
        exit 1
    fi
fi

if ! command -v pio >/dev/null; then
    echo "Error: 'pio' not found. Install with:"
    echo "  brew install platformio"
    exit 1
fi

echo "=== Flashing Clawdmeter (CYD) ==="
echo "Port: $PORT"
echo "Hold BOOT, press+release RST, release BOOT to enter flash mode."
echo ""

cd "$SCRIPT_DIR/firmware"
pio run -e "$BOARD" -t upload --upload-port "$PORT"

echo ""
echo "=== Done ==="
echo "Monitor with: pio device monitor -p $PORT -b 115200"

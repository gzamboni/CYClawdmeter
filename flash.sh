#!/bin/bash
# Build and flash Clawdmeter firmware on Linux (CYD / ESP32-2432S028R).
# Usage:
#   ./flash.sh                  # default port /dev/ttyUSB0
#   ./flash.sh /dev/ttyUSB1     # explicit USB serial port
#
# CYD uses CH340 USB-serial. Enter flash mode: hold BOOT, press+release RST, release BOOT.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BOARD="esp32_2432s028r"
PORT="${1:-/dev/ttyUSB0}"

echo "=== Flashing Clawdmeter (CYD) ==="
echo "Port: $PORT"
echo "Hold BOOT, press+release RST, release BOOT to enter flash mode."
echo ""

cd "$SCRIPT_DIR/firmware"
~/.platformio/penv/bin/pio run -e "$BOARD" -t upload --upload-port "$PORT"

echo ""
echo "=== Done! ==="

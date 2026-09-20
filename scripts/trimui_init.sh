#!/bin/sh
# TRIMUI Model S / Powkiddy A66 Launcher Hook Script
# Place this file in the root directory of the MicroSD card (/mnt/SDCARD/trimui_init.sh)

APP_DIR="/mnt/SDCARD/apps/kurui"

if [ -d "$APP_DIR" ] && [ -f "$APP_DIR/kurui" ]; then
    cd "$APP_DIR"
    chmod +x ./kurui
    # Replace the current shell process with kurui
    exec ./kurui
fi

# Fallback to standard MainUI if kurui is not present
if [ -f "/usr/trimui/bin/MainUI" ]; then
    exec /usr/trimui/bin/MainUI
fi

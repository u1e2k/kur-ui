#!/bin/sh
# TRIMUI Model S / Powkiddy A66 Launcher Hook Script
# Place this file in the root directory of the MicroSD card (/mnt/SDCARD/trimui_init.sh)

APP_DIR="/mnt/SDCARD/apps/kurui"
LOG_FILE="/mnt/SDCARD/kurui.log"

export LD_LIBRARY_PATH="/usr/lib:/usr/trimui/lib:/mnt/SDCARD/apps/kurui:$LD_LIBRARY_PATH"

if [ -d "$APP_DIR" ] && [ -f "$APP_DIR/kurui" ]; then
    cd "$APP_DIR"
    chmod +x ./kurui
    echo "=== KURUI LAUNCH ATTEMPT: $(date) ===" > "$LOG_FILE"
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >> "$LOG_FILE"
    # Execute kurui and capture stderr/stdout
    ./kurui >> "$LOG_FILE" 2>&1
    RET=$?
    echo "=== KURUI EXITED WITH CODE: $RET ===" >> "$LOG_FILE"
    sync
fi

# Fallback to standard MainUI so the screen never stays black
if [ -f "/usr/trimui/bin/MainUI" ]; then
    echo "Falling back to MainUI..." >> "$LOG_FILE"
    sync
    exec /usr/trimui/bin/MainUI
fi

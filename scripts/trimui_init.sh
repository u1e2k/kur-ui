#!/bin/sh
# TRIMUI Model S / Powkiddy A66 Launcher Hook Script
# Place this file in the root directory of the MicroSD card (/mnt/SDCARD/trimui_init.sh)

SD_ROOT="/mnt/SDCARD"
APP_DIR="$SD_ROOT/Apps/kurui"
LOG_FILE="$SD_ROOT/kurui.log"

export LD_LIBRARY_PATH="$APP_DIR:$SD_ROOT/lib:/usr/lib:/usr/trimui/lib:$LD_LIBRARY_PATH"

if [ -d "$APP_DIR" ] && [ -f "$APP_DIR/kurui" ]; then
    cd "$APP_DIR"
    chmod +x ./kurui
    echo "=== KURUI LAUNCH ATTEMPT: $(date) ===" > "$LOG_FILE"
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >> "$LOG_FILE"
    ./kurui >> "$LOG_FILE" 2>&1
    RET=$?
    echo "=== KURUI EXITED WITH CODE: $RET ===" >> "$LOG_FILE"
    sync
fi

# Fallback to standard MainUI if kurui exited or crashed
if [ -f "/usr/trimui/bin/MainUI" ]; then
    echo "Falling back to MainUI..." >> "$LOG_FILE"
    sync
    exec /usr/trimui/bin/MainUI
fi

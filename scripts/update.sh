#!/bin/sh
# TRIMUI Model S / Powkiddy A66 Launcher Runner
# Triggered when selected via TRIMUI "FILE" menu

killall updateui
killall keymon

SD_ROOT="/mnt/SDCARD"
INSTALL_DIR="$SD_ROOT/Apps/kurui"
UPDATE_DIR="$(dirname "$0")"

# 1. Update binary if bundled in zip
mkdir -p "$INSTALL_DIR"
if [ -f "$UPDATE_DIR/kurui" ]; then
    cp "$UPDATE_DIR/kurui" "$INSTALL_DIR/kurui"
    chmod +x "$INSTALL_DIR/kurui"
elif [ -f "$UPDATE_DIR/Apps/kurui/kurui" ]; then
    cp "$UPDATE_DIR/Apps/kurui/kurui" "$INSTALL_DIR/kurui"
    chmod +x "$INSTALL_DIR/kurui"
fi

# 2. Setup library path
export LD_LIBRARY_PATH="$INSTALL_DIR:$SD_ROOT/lib:/usr/lib:/usr/trimui/lib:$LD_LIBRARY_PATH"

# 3. Launch KURUI directly on the screen!
cd "$INSTALL_DIR"
if [ -f "./kurui" ]; then
    chmod +x ./kurui
    ./kurui > "$SD_ROOT/kurui.log" 2>&1
fi

# 4. Fallback to MainUI when KURUI exits
if [ -f "/usr/trimui/bin/MainUI" ]; then
    exec /usr/trimui/bin/MainUI
fi

#!/bin/sh
# TRIMUI Model S / Powkiddy A66 Safe Installer Script
# Integrates with TrimUI onboard 'notify' UI for native progress display

SD_ROOT="/mnt/SDCARD"
INSTALL_DIR="$SD_ROOT/Apps/kurui"
UPDATE_DIR="$(dirname "$0")"

do_notify() {
    pct="$1"
    txt="$2"
    if command -v notify >/dev/null 2>&1; then
        notify "$pct" update "$txt"
    else
        echo "[$pct%] $txt"
    fi
}

do_notify 10 "Preparing installation..."
mkdir -p "$INSTALL_DIR"

do_notify 30 "Copying KURUI binary..."
SOURCE_BIN=""
if [ -f "$UPDATE_DIR/kurui" ]; then
    SOURCE_BIN="$UPDATE_DIR/kurui"
elif [ -f "$UPDATE_DIR/Apps/kurui/kurui" ]; then
    SOURCE_BIN="$UPDATE_DIR/Apps/kurui/kurui"
elif [ -f "$UPDATE_DIR/apps/kurui/kurui" ]; then
    SOURCE_BIN="$UPDATE_DIR/apps/kurui/kurui"
fi

if [ -n "$SOURCE_BIN" ] && [ -s "$SOURCE_BIN" ]; then
    cp "$SOURCE_BIN" "$INSTALL_DIR/kurui"
    chmod +x "$INSTALL_DIR/kurui"
fi

do_notify 60 "Setting up boot hook..."
if [ -f "$UPDATE_DIR/trimui_init.sh" ]; then
    cp "$UPDATE_DIR/trimui_init.sh" "$SD_ROOT/trimui_init.sh"
    chmod +x "$SD_ROOT/trimui_init.sh"
fi

do_notify 80 "Cleaning up temporary files..."
rm -f "$0" "$SD_ROOT/updater" "$SD_ROOT/update.sh"
sync

do_notify 100 "Install complete! Rebooting..."
sleep 1

if command -v notify >/dev/null 2>&1; then
    notify 100 quit
fi

sleep 1
sync
reboot

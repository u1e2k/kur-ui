#!/bin/sh
# TRIMUI Model S / Powkiddy A66 Installer Script
# Triggered automatically when placed in SDCARD root as update.sh

SD_ROOT="/mnt/SDCARD"
INSTALL_DIR="$SD_ROOT/apps/kurui"
UPDATE_DIR="$(dirname "$0")"

echo "=== KURUI INSTALLER ==="

# Create application directory
mkdir -p "$INSTALL_DIR"

# Copy launcher binary and assets
if [ -f "$UPDATE_DIR/kurui" ]; then
    cp "$UPDATE_DIR/kurui" "$INSTALL_DIR/kurui"
    chmod +x "$INSTALL_DIR/kurui"
    echo "Installed: $INSTALL_DIR/kurui"
fi

# Copy launch hook script to SD root
if [ -f "$UPDATE_DIR/trimui_init.sh" ]; then
    cp "$UPDATE_DIR/trimui_init.sh" "$SD_ROOT/trimui_init.sh"
    chmod +x "$SD_ROOT/trimui_init.sh"
    echo "Installed: $SD_ROOT/trimui_init.sh"
elif [ -f "$UPDATE_DIR/scripts/trimui_init.sh" ]; then
    cp "$UPDATE_DIR/scripts/trimui_init.sh" "$SD_ROOT/trimui_init.sh"
    chmod +x "$SD_ROOT/trimui_init.sh"
    echo "Installed: $SD_ROOT/trimui_init.sh"
fi

# Self-cleanup of installer to prevent endless re-installation loop
rm -f "$SD_ROOT/update.sh"
sync

echo "KURUI installation completed successfully. Rebooting..."
reboot

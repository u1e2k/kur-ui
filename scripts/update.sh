#!/bin/sh
# TRIMUI Model S / Powkiddy A66 Safe Installer Script
# Triggered automatically when placed in SDCARD root as update.sh

SD_ROOT="/mnt/SDCARD"
INSTALL_DIR="$SD_ROOT/apps/kurui"
UPDATE_DIR="$(dirname "$0")"

echo "=== KURUI SAFE INSTALLER ==="

# 1. Binary existence validation (prevent bricking / black-screen loop)
SOURCE_BIN=""
if [ -f "$UPDATE_DIR/kurui" ]; then
    SOURCE_BIN="$UPDATE_DIR/kurui"
elif [ -f "$UPDATE_DIR/apps/kurui/kurui" ]; then
    SOURCE_BIN="$UPDATE_DIR/apps/kurui/kurui"
fi

if [ -z "$SOURCE_BIN" ] || [ ! -s "$SOURCE_BIN" ]; then
    echo "[ERROR] kurui binary not found or empty! Aborting installation safely."
    rm -f "$0"
    sync
    exit 1
fi

# 2. Create application target directory
mkdir -p "$INSTALL_DIR"

# 3. Copy launcher binary
cp "$SOURCE_BIN" "$INSTALL_DIR/kurui"
chmod +x "$INSTALL_DIR/kurui"
echo "[OK] Installed binary to $INSTALL_DIR/kurui"

# 4. Backup existing trimui_init.sh if present
if [ -f "$SD_ROOT/trimui_init.sh" ]; then
    cp "$SD_ROOT/trimui_init.sh" "$SD_ROOT/trimui_init.sh.bak"
    echo "[BACKUP] Preserved existing trimui_init.sh as trimui_init.sh.bak"
fi

# 5. Install new trimui_init.sh hook script
SOURCE_HOOK=""
if [ -f "$UPDATE_DIR/trimui_init.sh" ]; then
    SOURCE_HOOK="$UPDATE_DIR/trimui_init.sh"
elif [ -f "$UPDATE_DIR/scripts/trimui_init.sh" ]; then
    SOURCE_HOOK="$UPDATE_DIR/scripts/trimui_init.sh"
fi

if [ -n "$SOURCE_HOOK" ] && [ -f "$SOURCE_HOOK" ]; then
    cp "$SOURCE_HOOK" "$SD_ROOT/trimui_init.sh"
    chmod +x "$SD_ROOT/trimui_init.sh"
    echo "[OK] Installed hook to $SD_ROOT/trimui_init.sh"
else
    # Fallback hook generation if missing
    cat << 'EOF' > "$SD_ROOT/trimui_init.sh"
#!/bin/sh
APP_DIR="/mnt/SDCARD/apps/kurui"
if [ -d "$APP_DIR" ] && [ -f "$APP_DIR/kurui" ]; then
    cd "$APP_DIR"
    chmod +x ./kurui
    exec ./kurui
fi
if [ -f "/usr/trimui/bin/MainUI" ]; then
    exec /usr/trimui/bin/MainUI
fi
EOF
    chmod +x "$SD_ROOT/trimui_init.sh"
    echo "[OK] Generated fallback hook at $SD_ROOT/trimui_init.sh"
fi

# 6. Self-cleanup to prevent endless re-install loop
echo "[CLEANUP] Removing installer: $0"
rm -f "$0"

# 7. Safe disk flush (guarantee slow SD writes complete)
echo "[SYNC] Flushing disk buffers..."
sync
sleep 2
sync

echo "[REBOOT] Installation completed successfully. Rebooting to KURUI..."
reboot

#!/usr/bin/env bash
set -euo pipefail

# Sentinel macOS Silent Uninstaller & Cleanup Script

echo "=== Sentinel macOS Uninstaller ==="

# 1. Terminate running process
echo "Stopping Sentinel Desktop process..."
pkill -x "sentinel-desktop" || true

# 2. Remove App Bundle
if [ -d "/Applications/Sentinel Desktop.app" ]; then
    echo "Removing /Applications/Sentinel Desktop.app..."
    rm -rf "/Applications/Sentinel Desktop.app"
fi

if [ -d "${HOME}/Applications/Sentinel Desktop.app" ]; then
    echo "Removing ~/Applications/Sentinel Desktop.app..."
    rm -rf "${HOME}/Applications/Sentinel Desktop.app"
fi

# 3. Clean Preferences & Application Support
echo "Cleaning application preferences and support files..."
rm -rf "${HOME}/Library/Application Support/Sopwit/Sentinel Desktop"
rm -rf "${HOME}/Library/Logs/Sentinel"
rm -rf "${HOME}/Library/Caches/dev.sentinel.Sentinel"
rm -f "${HOME}/Library/Preferences/dev.sentinel.Sentinel.plist"

# 4. Remove Keychain item (optional cleanup)
security delete-generic-password -s "dev.sentinel.Sentinel" -a "master_encryption_key" || true

echo "✅ Sentinel Desktop has been completely uninstalled."

#!/usr/bin/env bash
set -euo pipefail

# Sentinel macOS PKG Installer Builder
# Usage: ./packaging/macos/build_pkg.sh [APP_PATH] [OUTPUT_PKG] [DEVELOPER_ID_INSTALLER_CERT]

APP_PATH="${1:-build/apps/sentinel-desktop/Sentinel Desktop.app}"
OUTPUT_PKG="${2:-build/SentinelDesktop.pkg}"
SIGNING_IDENTITY="${3:-}"

echo "=== Building Enterprise PKG for Sentinel Desktop ==="
echo "App Path: ${APP_PATH}"
echo "Output PKG: ${OUTPUT_PKG}"

if [ ! -d "${APP_PATH}" ]; then
    echo "Error: App bundle not found at ${APP_PATH}"
    exit 1
fi

TEMP_PKG_DIR=$(mktemp -d /tmp/sentinel_pkg.XXXXXX)
trap 'rm -rf "${TEMP_PKG_DIR}"' EXIT

COMPONENT_PKG="${TEMP_PKG_DIR}/component.pkg"
COMPONENT_PLIST="${TEMP_PKG_DIR}/component.plist"

# 1. Build component PKG targeting /Applications
cat > "${COMPONENT_PLIST}" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<array>
    <dict>
        <key>RootRelativeBundlePath</key>
        <string></string>
        <key>BundleIsRelocatable</key>
        <false/>
        <key>BundleIsVersionChecked</key>
        <true/>
        <key>BundleHasStrictIdentifier</key>
        <true/>
        <key>BundleOverwriteAction</key>
        <string>upgrade</string>
    </dict>
</array>
</plist>
EOF

pkgbuild \
    --root "${APP_PATH}" \
    --install-location "/Applications/Sentinel Desktop.app" \
    --component-plist "${COMPONENT_PLIST}" \
    "${COMPONENT_PKG}"

# 2. Build final distribution PKG
if [ -n "${SIGNING_IDENTITY}" ]; then
    echo "Signing PKG with Developer ID Installer: ${SIGNING_IDENTITY}"
    productbuild \
        --package "${COMPONENT_PKG}" "/Applications" \
        --sign "${SIGNING_IDENTITY}" \
        "${OUTPUT_PKG}"
else
    echo "Warning: No Developer ID Installer certificate specified. Creating unsigned PKG."
    productbuild \
        --package "${COMPONENT_PKG}" "/Applications" \
        "${OUTPUT_PKG}"
fi

echo "✅ PKG Successfully Created: ${OUTPUT_PKG}"

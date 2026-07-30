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

# 1. Build component PKG targeting /Applications
pkgbuild \
    --root "${APP_PATH}" \
    --install-location "/Applications/Sentinel Desktop.app" \
    --component-plist <(cat <<EOF
[
  {
    "RootRelativeBundlePath": "",
    "BundleIsRelocatable": false,
    "BundleIsVersionChecked": true,
    "BundleHasStrictIdentifier": true,
    "BundleOverwriteAction": "upgrade"
  }
]
EOF
) \
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

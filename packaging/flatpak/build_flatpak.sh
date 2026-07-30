#!/usr/bin/env bash
# ==============================================================================
# Sentinel Local Flatpak Build & Test Automation Script
# Uses flatpak-builder to build, install, and run Sentinel in a Flatpak sandbox
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build-flatpak"
REPO_DIR="${REPO_ROOT}/build-flatpak-repo"

MANIFEST_YAML="${SCRIPT_DIR}/org.sentinel.Sentinel.yml"
if [ ! -f "${MANIFEST_YAML}" ]; then
    MANIFEST_YAML="${SCRIPT_DIR}/dev.sentinel.Sentinel.yaml"
fi

echo "==> Building Sentinel Flatpak package locally..."
echo "    Manifest : ${MANIFEST_YAML}"
echo "    Build Dir: ${BUILD_DIR}"

if ! command -v flatpak-builder &> /dev/null; then
    echo "[ERROR] flatpak-builder is not installed. Please install 'flatpak-builder' first."
    exit 1
fi

if [[ "${1:-}" == "--clean" ]]; then
    echo "==> Cleaning previous Flatpak build directory..."
    rm -rf "${BUILD_DIR}" "${REPO_DIR}"
    shift || true
fi

echo "==> Invoking flatpak-builder..."
flatpak-builder \
    --force-clean \
    --repo="${REPO_DIR}" \
    "${BUILD_DIR}" \
    "${MANIFEST_YAML}"

echo "==> Flatpak build successful!"

if [[ "${1:-}" == "--run" ]]; then
    echo "==> Running Sentinel inside Flatpak sandbox..."
    flatpak-builder --run "${BUILD_DIR}" "${MANIFEST_YAML}" sentinel-desktop
elif [[ "${1:-}" == "--install" ]]; then
    echo "==> Installing Sentinel to local user Flatpak repository..."
    flatpak --user remote-add --if-not-exists --no-gpg-verify sentinel-local-repo "${REPO_DIR}"
    flatpak --user install -y sentinel-local-repo dev.sentinel.Sentinel || flatpak --user update -y dev.sentinel.Sentinel
    echo "==> Installation complete! Launch with: flatpak run dev.sentinel.Sentinel"
fi

#!/usr/bin/env bash
# ==============================================================================
# Sentinel Fedora COPR Packaging & Deployment Automation
# Target OS: Fedora Linux (KDE Plasma 6)
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build-copr"
SPEC_FILE="${SCRIPT_DIR}/sentinel-desktop.spec"
COPR_PROJECT="${COPR_PROJECT:-sentinel/sentinel-desktop}"

echo "==> Initializing Fedora COPR package build for Sentinel..."
echo "    Repo Root : ${REPO_ROOT}"
echo "    Spec File : ${SPEC_FILE}"
echo "    Target Repo: ${COPR_PROJECT}"

# Extract version from spec file
VERSION=$(grep -E '^Version:' "${SPEC_FILE}" | awk '{print $2}')
NAME=$(grep -E '^Name:' "${SPEC_FILE}" | awk '{print $2}')
TARBALL_NAME="${NAME}-${VERSION}.tar.gz"

echo "    Package   : ${NAME}-${VERSION}"

# Create clean build output directory
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}/SOURCES" "${BUILD_DIR}/SPECS" "${BUILD_DIR}/SRPMS"

# Prepare Source Tarball
echo "==> Creating source tarball (${TARBALL_NAME})..."
git -C "${REPO_ROOT}" archive --prefix="${NAME}-${VERSION}/" --format=tar.gz HEAD > "${BUILD_DIR}/SOURCES/${TARBALL_NAME}"

# Copy Spec file
cp "${SPEC_FILE}" "${BUILD_DIR}/SPECS/"

# Validate Spec file using rpmlint if available
if command -v rpmlint &> /dev/null; then
    echo "==> Linting RPM Spec file with rpmlint..."
    rpmlint "${BUILD_DIR}/SPECS/sentinel-desktop.spec" || true
else
    echo "    [NOTE] rpmlint not found; skipping lint step."
fi

# Build Source RPM (SRPM)
if command -v rpmbuild &> /dev/null; then
    echo "==> Building Source RPM (SRPM)..."
    rpmbuild -bs \
        --define "_topdir ${BUILD_DIR}" \
        "${BUILD_DIR}/SPECS/sentinel-desktop.spec"
    echo "==> SRPM created successfully in ${BUILD_DIR}/SRPMS/"
else
    echo "    [NOTE] rpmbuild not found; SRPM generation skipped."
fi

# Submit build to Fedora COPR if copr-cli is installed and COPR_SUBMIT is enabled
if [[ "${1:-}" == "--submit" ]]; then
    if command -v copr-cli &> /dev/null; then
        SRPM_FILE=$(find "${BUILD_DIR}/SRPMS" -name "*.src.rpm" | head -n 1)
        if [[ -n "${SRPM_FILE}" ]]; then
            echo "==> Submitting SRPM to Fedora COPR (${COPR_PROJECT})..."
            copr-cli build "${COPR_PROJECT}" "${SRPM_FILE}"
        else
            echo "==> Submitting build to Fedora COPR from Git repository..."
            copr-cli buildscm "${COPR_PROJECT}" \
                --clone-url "https://github.com/sentinel/sentinel.git" \
                --spec "packaging/linux/fedora-kde/sentinel-desktop.spec" \
                --type git
        fi
    else
        echo "[ERROR] copr-cli is not installed. Please install 'copr-cli' to submit builds to Fedora COPR."
        exit 1
    fi
else
    echo "==> COPR build preparation complete!"
    echo "    To submit to Fedora COPR, run: ${0} --submit"
fi

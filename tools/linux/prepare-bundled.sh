#!/bin/bash
# prepare-bundled.sh — Generate bundled/openclaw.tgz from installed openclaw
# Run this before building the release zip.
#
# Usage: bash tools/linux/prepare-bundled.sh
#
# Requirements: openclaw installed globally (npm install -g openclaw)

set -e
cd "$(dirname "$0")/../.."

BUNDLED_DIR="bundled"
TARBALL="${BUNDLED_DIR}/openclaw.tgz"

mkdir -p "${BUNDLED_DIR}"

echo "=== Preparing bundled OpenClaw package ==="

# Check if openclaw is installed
if ! command -v openclaw &>/dev/null; then
    echo "ERROR: openclaw not found. Install it first: npm install -g openclaw"
    exit 1
fi

# Get installed openclaw version
OC_VER=$(openclaw --version 2>/dev/null | tr -d '\r\n ')
echo "OpenClaw version: ${OC_VER}"

# Find the global npm package path
OC_PATH=$(npm root -g 2>/dev/null)/openclaw
if [ ! -d "${OC_PATH}" ]; then
    echo "ERROR: OpenClaw package not found at ${OC_PATH}"
    exit 1
fi

# Pack it
echo "Packing ${OC_PATH} → ${TARBALL}"
cd "${OC_PATH}"
npm pack 2>/dev/null
TARBALL_NAME=$(ls openclaw-*.tgz 2>/dev/null | head -1)

if [ -z "${TARBALL_NAME}" ]; then
    echo "ERROR: npm pack produced no tarball"
    exit 1
fi

# Move to bundled dir
cd -
mv "${OC_PATH}/${TARBALL_NAME}" "${TARBALL}"
ls -lh "${TARBALL}"
echo "=== Bundled package ready: ${TARBALL} ==="

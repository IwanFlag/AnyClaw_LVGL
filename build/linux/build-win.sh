#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION="${1:-v2.0.0}"
BUILD_DIR="${ROOT_DIR}/build/linux/out"
TOOLCHAIN="${ROOT_DIR}/build/toolchain-mingw64.cmake"
EXE_PATH="${BUILD_DIR}/bin/AnyClaw_LVGL.exe"
SDL_PATH="${BUILD_DIR}/bin/SDL2.dll"
ARTIFACTS_DIR="${ROOT_DIR}/build/linux/artifacts"

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j"$(nproc)"

# Package
mkdir -p "${ARTIFACTS_DIR}"
cp -f "${EXE_PATH}" "${ARTIFACTS_DIR}/"
cp -f "${SDL_PATH}" "${ARTIFACTS_DIR}/"
if [ -d "${ROOT_DIR}/assets" ]; then
  mkdir -p "${ARTIFACTS_DIR}/assets"
  cp -f "${ROOT_DIR}/assets/"*.png "${ARTIFACTS_DIR}/assets/" 2>/dev/null || true
fi

TIMESTAMP=$(date '+%Y%m%d_%H%M')
ZIP_NAME="AnyClaw_LVGL_${VERSION}_${TIMESTAMP}.zip"
cd "${ARTIFACTS_DIR}" && zip -r "${ARTIFACTS_DIR}/${ZIP_NAME}" .
echo "[OK] Package: build/linux/artifacts/${ZIP_NAME}"

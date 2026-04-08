#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-linux"
ARTIFACTS_DIR="${ROOT_DIR}/artifacts/linux-native"

bash "${ROOT_DIR}/tools/linux/setup-env.sh"

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j"$(nproc)"

mkdir -p "${ARTIFACTS_DIR}"
cp -f "${BUILD_DIR}/bin/AnyClaw_LVGL" "${ARTIFACTS_DIR}/AnyClaw_LVGL" 2>/dev/null || true
cp -f "${BUILD_DIR}/bin/AnyClaw_LVGL.exe" "${ARTIFACTS_DIR}/AnyClaw_LVGL.exe" 2>/dev/null || true
echo "[OK] Linux native build done: ${ARTIFACTS_DIR}"

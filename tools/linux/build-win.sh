#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-mingw"
TOOLCHAIN="${ROOT_DIR}/tools/linux/toolchain-mingw64.cmake"
SDL2_PREFIX="${1:-/opt/sdl2-mingw}"
ARTIFACTS_DIR="${ROOT_DIR}/artifacts/linux-to-windows"
EXE_PATH="${BUILD_DIR}/bin/AnyClaw_LVGL.exe"
SDL_PATH="${BUILD_DIR}/bin/SDL2.dll"

bash "${ROOT_DIR}/tools/linux/setup-env.sh"

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

CMAKE_ARGS=("-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN}" "-DCMAKE_BUILD_TYPE=Release")
if [ -d "${SDL2_PREFIX}" ]; then
  CMAKE_ARGS+=("-DSDL2_CROSS_PREFIX=${SDL2_PREFIX}")
fi

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" "${CMAKE_ARGS[@]}"
cmake --build "${BUILD_DIR}" -j"$(nproc)"

bash "${ROOT_DIR}/tools/common/package_windows_full_runtime.sh" \
  "${ROOT_DIR}" "${EXE_PATH}" "${SDL_PATH}" "${ARTIFACTS_DIR}"

echo "[OK] Linux->Windows build/package done: ${ARTIFACTS_DIR}"

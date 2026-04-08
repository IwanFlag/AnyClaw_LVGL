#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="${1:?project dir required}"
EXE_PATH="${2:?exe path required}"
SDL_PATH="${3:?sdl2 path required}"
OUT_ROOT="${4:?out root required}"

[ -d "${PROJECT_DIR}" ] || { echo "Project dir not found: ${PROJECT_DIR}"; exit 1; }
[ -f "${EXE_PATH}" ] || { echo "Exe not found: ${EXE_PATH}"; exit 1; }
[ -f "${SDL_PATH}" ] || { echo "SDL2.dll not found: ${SDL_PATH}"; exit 1; }

mkdir -p "${OUT_ROOT}"
TS="$(date +%Y%m%d_%H%M%S)"
PKG_DIR="${OUT_ROOT}/AnyClaw_LVGL_windows_full-runtime_${TS}"
mkdir -p "${PKG_DIR}"

cp -f "${EXE_PATH}" "${PKG_DIR}/AnyClaw_LVGL.exe"
cp -f "${SDL_PATH}" "${PKG_DIR}/SDL2.dll"
cp -r "${PROJECT_DIR}/assets" "${PKG_DIR}/assets"
cp -r "${PROJECT_DIR}/bundled" "${PKG_DIR}/bundled"

ZIP_PATH="${PKG_DIR}.zip"
rm -f "${ZIP_PATH}"
(cd "${PKG_DIR}" && zip -r "${ZIP_PATH}" . >/dev/null)
echo "${ZIP_PATH}"

#!/usr/bin/env bash
# Cross-compile FreeType for MinGW (Windows x64)
# Produces: thirdparty/freetype-mingw/{include/,lib/libfreetype.a}
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
FT_VERSION="2.13.3"
FT_SRC="freetype-${FT_VERSION}"
FT_URL="https://download.savannah.gnu.org/releases/freetype/${FT_SRC}.tar.gz"
INSTALL_DIR="${ROOT_DIR}/thirdparty/freetype-mingw"
BUILD_DIR="/tmp/freetype-mingw-build"

echo "=== FreeType MinGW Cross-Compile ==="
echo "Version: ${FT_VERSION}"
echo "Install: ${INSTALL_DIR}"

# Download
mkdir -p "${BUILD_DIR}"
if [ ! -f "${BUILD_DIR}/${FT_SRC}.tar.gz" ]; then
    echo "Downloading FreeType ${FT_VERSION}..."
    wget -q --show-progress -O "${BUILD_DIR}/${FT_SRC}.tar.gz" "${FT_URL}" || \
    curl -L -o "${BUILD_DIR}/${FT_SRC}.tar.gz" "${FT_URL}"
fi

# Extract
if [ ! -d "${BUILD_DIR}/${FT_SRC}" ]; then
    echo "Extracting..."
    tar xzf "${BUILD_DIR}/${FT_SRC}.tar.gz" -C "${BUILD_DIR}"
fi

# Build
cd "${BUILD_DIR}/${FT_SRC}"
echo "Configuring CMake (MinGW cross-compile)..."
cmake -B build-mingw \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    -DBUILD_SHARED_LIBS=OFF \
    -DFT_DISABLE_BZIP2=ON \
    -DFT_DISABLE_BROTLI=ON \
    -DFT_DISABLE_PNG=ON \
    -DFT_DISABLE_HARFBUZZ=ON \
    -DFT_DISABLE_ZLIB=ON \
    -DCMAKE_BUILD_TYPE=Release

echo "Building..."
cmake --build build-mingw -j$(nproc)

echo "Installing..."
cmake --install build-mingw

echo "=== Done ==="
echo "Headers: ${INSTALL_DIR}/include/freetype2/"
echo "Library: ${INSTALL_DIR}/lib/libfreetype.a"
ls -la "${INSTALL_DIR}/lib/libfreetype.a" 2>/dev/null && echo "✅ Build successful" || echo "❌ Build failed"

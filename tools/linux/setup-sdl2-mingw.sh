#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  AnyClaw LVGL — 交叉编译 SDL2 for MinGW
#  用法: bash tools/linux/setup-sdl2-mingw.sh [install_prefix]
#  默认安装路径: /opt/sdl2-mingw
# ═══════════════════════════════════════════════════════════════
set -e

SDL2_VERSION="2.30.3"
INSTALL_PREFIX="${1:-/opt/sdl2-mingw}"
BUILD_DIR="/tmp/SDL2-build-$$"

echo "════════════════════════════════════════════════════════"
echo "  SDL2 ${SDL2_VERSION} Cross-Compile for MinGW"
echo "  Install prefix: ${INSTALL_PREFIX}"
echo "════════════════════════════════════════════════════════"

# 1. 下载 SDL2 源码
echo "[1/3] Cloning SDL2 ${SDL2_VERSION}..."
# 优化 git 网络配置（GitHub 大仓库克隆可能断连）
git config --global http.postBuffer 524288000 2>/dev/null || true
if [ ! -d /tmp/SDL2-src ]; then
    git clone --depth 1 https://github.com/libsdl-org/SDL.git -b "release-${SDL2_VERSION}" /tmp/SDL2-src
else
    echo "  ✓ SDL2 source already exists at /tmp/SDL2-src"
fi

# 2. 交叉编译
echo "[2/3] Cross-compiling SDL2..."
mkdir -p "${BUILD_DIR}" && cd "${BUILD_DIR}"
cmake /tmp/SDL2-src \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DSDL_SHARED=ON \
    -DSDL_STATIC=OFF \
    -DSDL_TEST=OFF \
    -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
make install
echo "  ✓ SDL2 built and installed to ${INSTALL_PREFIX}"

# 3. 清理
rm -rf "${BUILD_DIR}"

echo ""
echo "════════════════════════════════════════════════════════"
echo "  ✅ SDL2 ${SDL2_VERSION} installed to ${INSTALL_PREFIX}"
echo "  Build AnyClaw with:"
echo "    cmake .. -DCMAKE_TOOLCHAIN_FILE=../tools/linux/toolchain-mingw64.cmake \\"
echo "             -DSDL2_CROSS_PREFIX=${INSTALL_PREFIX}"
echo "════════════════════════════════════════════════════════"

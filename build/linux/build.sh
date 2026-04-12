#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  AnyClaw LVGL — 一键交叉编译 + 打包 + 截图 (Linux → Windows)
#  用法: bash build/linux/build.sh [version]
#  示例: bash build/linux/build.sh v2.0.9
# ═══════════════════════════════════════════════════════════════
set -e

PROJECT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION="${1:-v2.0.0}"
BUILD_DIR="${PROJECT_DIR}/out/mingw"
TOOLCHAIN="${PROJECT_DIR}/build/toolchain-mingw64.cmake"
BIN_DIR="${BUILD_DIR}/bin"

echo "════════════════════════════════════════════════════════"
echo "  AnyClaw LVGL — Cross-Compile (Linux → Windows x64)"
echo "  Project:  ${PROJECT_DIR}"
echo "  SDL2:     ${SDL2_PREFIX}"
echo "  Build:    ${BUILD_DIR}"
echo "  Version:  ${VERSION}"
echo "════════════════════════════════════════════════════════"

# 1. 清理旧构建
echo "[1/5] Cleaning old build..."
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

# 2. CMake 配置
echo "[2/5] Configuring..."
cd "${BUILD_DIR}"
CMAKE_ARGS="-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN} -DCMAKE_BUILD_TYPE=Release"
echo "  Using bundled SDL2 at thirdparty/sdl2-mingw/"

cmake ${PROJECT_DIR} ${CMAKE_ARGS}

# 3. 编译
echo "[3/5] Building..."
make -j$(nproc)

# 3.5 复制 assets 到 build 目录（Wine 测试需要）
echo "[3.5] Copying assets to build dir..."
if [ -d "${PROJECT_DIR}/assets" ]; then
    mkdir -p "${BIN_DIR}/assets/tray" "${BIN_DIR}/assets/icons/ai"
    cp "${PROJECT_DIR}/assets/"*.png "${BIN_DIR}/assets/" 2>/dev/null || true
    cp "${PROJECT_DIR}/assets/tray/"*.png "${BIN_DIR}/assets/tray/" 2>/dev/null || true
    cp "${PROJECT_DIR}/assets/icons/ai/"*.png "${BIN_DIR}/assets/icons/ai/" 2>/dev/null || true
    echo "  ✓ Assets copied to ${BIN_DIR}/assets/"
fi

# 3.6 代码签名
echo "[3.6] Code signing..."
PFX_FILE="${BUILD_DIR}/sign.pfx"
if [ -f "${PFX_FILE}" ]; then
    if command -v osslsigncode &>/dev/null; then
        cd "${BIN_DIR}"
        osslsigncode sign -pkcs12 "${PFX_FILE}" -pass anyclaw \
            -n "AnyClaw LVGL" -i "https://github.com/IwanFlag/AnyClaw_LVGL" \
            -t http://timestamp.digicert.com \
            -in AnyClaw_LVGL.exe -out AnyClaw_LVGL_signed.exe
        mv AnyClaw_LVGL_signed.exe AnyClaw_LVGL.exe
        echo "  🔐 Signed OK"
    else
        echo "  ⚠️ osslsigncode not installed, skipping signing"
    fi
else
    echo "  ⚠️ ${PFX_FILE} not found, skipping signing"
fi

# 4. 打包 zip
echo "[4/5] Packaging..."
# 删除旧 zip
rm -f "${BIN_DIR}"/AnyClaw_LVGL_*.zip
if [ -f "${BIN_DIR}/AnyClaw_LVGL.exe" ]; then
    TIMESTAMP=$(date '+%Y%m%d_%H%M')
    ZIP_NAME="AnyClaw_LVGL_${VERSION}_${TIMESTAMP}.zip"
    cd "${BIN_DIR}"

    # 创建临时目录，构建目标结构
    PKG_DIR="${BIN_DIR}/_pkg"
    rm -rf "${PKG_DIR}"
    mkdir -p "${PKG_DIR}/assets"
    cp AnyClaw_LVGL.exe "${PKG_DIR}/"
    if [ -f "SDL2.dll" ]; then
        cp SDL2.dll "${PKG_DIR}/"
    else
        echo "  ⚠ SDL2.dll not found, package will need it at runtime"
    fi

    # 复制 assets/app_icon.png + garlic_icon.png + oc_icon + tray/*.png
    if [ -f "${PROJECT_DIR}/assets/app_icon.png" ]; then
        cp "${PROJECT_DIR}/assets/app_icon.png" "${PKG_DIR}/assets/"
        echo "  ✓ Included assets/app_icon.png"
    fi
    for f in garlic_icon.png garlic_32.png garlic_48.png oc_icon_24.png; do
        if [ -f "${PROJECT_DIR}/assets/$f" ]; then
            cp "${PROJECT_DIR}/assets/$f" "${PKG_DIR}/assets/"
            echo "  ✓ Included assets/$f"
        fi
    done
    if [ -d "${PROJECT_DIR}/assets/tray" ]; then
        mkdir -p "${PKG_DIR}/assets/tray"
        cp "${PROJECT_DIR}/assets/tray/"*.png "${PKG_DIR}/assets/tray/"
        echo "  ✓ Included assets/tray/ ($(ls "${PROJECT_DIR}/assets/tray/"*.png | wc -l) files)"
    fi
    if [ -d "${PROJECT_DIR}/assets/icons" ]; then
        mkdir -p "${PKG_DIR}/assets/icons/ai"
        cp "${PROJECT_DIR}/assets/icons/ai/"*.png "${PKG_DIR}/assets/icons/ai/"
        echo "  ✓ Included assets/icons/ai/ ($(ls "${PROJECT_DIR}/assets/icons/ai/"*.png | wc -l) files)"
    fi

    # 复制 bundled/openclaw.tgz (离线安装包)
    if [ -f "${PROJECT_DIR}/bundled/openclaw.tgz" ]; then
        mkdir -p "${PKG_DIR}/bundled"
        cp "${PROJECT_DIR}/bundled/openclaw.tgz" "${PKG_DIR}/bundled/"
        echo "  ✓ Included bundled/openclaw.tgz ($(du -h "${PROJECT_DIR}/bundled/openclaw.tgz" | cut -f1))"
    else
        echo "  ⚠ bundled/openclaw.tgz not found (run prepare-bundled.sh first)"
    fi

    # 复制 bundled/workspace/ (OpenClaw 工作区模板文件)
    if [ -d "${PROJECT_DIR}/bundled/workspace" ]; then
        mkdir -p "${PKG_DIR}/bundled/workspace"
        cp "${PROJECT_DIR}/bundled/workspace/"*.md "${PKG_DIR}/bundled/workspace/" 2>/dev/null
        WS_COUNT=$(ls "${PKG_DIR}/bundled/workspace/"*.md 2>/dev/null | wc -l)
        echo "  ✓ Included bundled/workspace/ (${WS_COUNT} template files)"
    else
        echo "  ⚠ bundled/workspace/ not found"
    fi

    # 打包
    cd "${PKG_DIR}"
    zip -r "${BIN_DIR}/${ZIP_NAME}" .
    rm -rf "${PKG_DIR}"
    echo "  ✅ Package: ${BIN_DIR}/${ZIP_NAME} ($(du -h "${BIN_DIR}/${ZIP_NAME}" | cut -f1))"
    echo "     Contents: AnyClaw_LVGL.exe + SDL2.dll + assets/app_icon.png"
else
    echo "  ❌ Binary not found, skipping package"
    exit 1
fi

# 5. Wine 截图
echo "[5/5] Capturing screenshot..."
DISPLAY_NUM=99
SCREENSHOT="${BIN_DIR}/screenshot_${VERSION}_${TIMESTAMP}.png"

# 启动 Xvfb
Xvfb :${DISPLAY_NUM} -screen 0 1280x720x24 &
XVFB_PID=$!
sleep 1

export DISPLAY=:${DISPLAY_NUM}
export WINEPREFIX="${HOME}/.wine"

# 运行并截图
cd "${BIN_DIR}"
timeout 8 wine AnyClaw_LVGL.exe 2>/dev/null &
WINE_PID=$!
sleep 4

# 截图
if command -v xwd &>/dev/null && command -v convert &>/dev/null; then
    xwd -root -display :${DISPLAY_NUM} -out /tmp/anyclaw.xwd 2>/dev/null && \
    convert /tmp/anyclaw.xwd "${SCREENSHOT}" 2>/dev/null && \
    echo "  ✅ Screenshot: ${SCREENSHOT}" || \
    echo "  ⚠️ Screenshot capture failed (non-critical)"
else
    echo "  ⚠️ xwd/imagemagick not installed, skipping screenshot"
fi

# 清理
kill ${WINE_PID} 2>/dev/null || true
kill ${XVFB_PID} 2>/dev/null || true
pkill -f AnyClaw_LVGL 2>/dev/null || true
rm -f /tmp/anyclaw.xwd

# 总结
echo ""
echo "════════════════════════════════════════════════════════"
echo "  ✅ Build complete!"
echo "  Binary:    ${BIN_DIR}/AnyClaw_LVGL.exe ($(du -h "${BIN_DIR}/AnyClaw_LVGL.exe" | cut -f1))"
if [ -f "${BIN_DIR}/SDL2.dll" ]; then
    echo "  DLL:       ${BIN_DIR}/SDL2.dll ($(du -h "${BIN_DIR}/SDL2.dll" | cut -f1))"
fi
echo "  Package:   ${BIN_DIR}/${ZIP_NAME}"
echo "  Layout:    AnyClaw_LVGL.exe / SDL2.dll / assets/app_icon.png"
if [ -f "${SCREENSHOT}" ]; then
    echo "  Screenshot: ${SCREENSHOT}"
fi
echo ""
echo "  Test:  bash build/linux/run-wine.sh"
echo "════════════════════════════════════════════════════════"

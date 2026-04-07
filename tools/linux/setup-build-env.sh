#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  AnyClaw LVGL — Linux 交叉编译环境一键搭建
#  用法: sudo bash tools/linux/setup-build-env.sh
#  适用: Ubuntu 22.04+ / Debian 12+
# ═══════════════════════════════════════════════════════════════
set -e

echo "════════════════════════════════════════════════════════"
echo "  AnyClaw LVGL — Build Environment Setup (Linux)"
echo "════════════════════════════════════════════════════════"

# 1. 切换可用镜像源（阿里云不通时切清华源）
echo "[1/5] Checking apt mirrors..."
# 先测试当前源是否可达
MIRROR_OK=false
if apt-get update -qq 2>/dev/null; then
    # 进一步验证能否真正下载包索引
    if apt-cache showpkg mingw-w64 2>/dev/null | grep -q "mingw-w64"; then
        MIRROR_OK=true
    fi
fi
if [ "$MIRROR_OK" = false ]; then
    echo "  Default mirror unreachable or missing packages, switching to Tsinghua mirror..."
    cat > /etc/apt/sources.list << 'MIRROR'
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ noble main restricted universe multiverse
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ noble-updates main restricted universe multiverse
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ noble-backports main restricted universe multiverse
deb https://mirrors.tuna.tsinghua.edu.cn/ubuntu/ noble-security main restricted universe multiverse
MIRROR
    apt-get update -qq
fi
echo "  ✓ apt mirrors OK"

# 1.5 配置 git 网络优化（GitHub 克隆大仓库可能断连）
echo "[1.5] Optimizing git network config..."
git config --global http.postBuffer 524288000 2>/dev/null || true
echo "  ✓ git http.postBuffer = 512MB"

# 2. 安装交叉编译工具链
echo "[2/5] Installing MinGW-w64 cross-compiler..."
apt-get install -y -qq mingw-w64 cmake pkg-config git
echo "  ✓ MinGW-w64 $(x86_64-w64-mingw32-gcc --version | head -1)"

# 3. 安装 Wine（用于本地测试）
echo "[3/5] Installing Wine..."
dpkg --add-architecture i386 2>/dev/null || true
apt-get update -qq 2>/dev/null
apt-get install -y -qq wine64 xvfb || apt-get install -y -qq wine xvfb
echo "  ✓ Wine $(wine --version 2>&1 | head -1)"

# 4. 安装辅助工具
echo "[4/5] Installing helper tools..."
apt-get install -y -qq zip imagemagick 2>/dev/null || true
echo "  ✓ Helper tools installed"

# 5. 验证
echo "[5/5] Verifying installation..."
echo "  CC:   $(x86_64-w64-mingw32-gcc --version | head -1)"
echo "  CXX:  $(x86_64-w64-mingw32-g++ --version | head -1)"
echo "  RC:   $(x86_64-w64-mingw32-windres --version | head -1)"
echo "  CMake: $(cmake --version | head -1)"
echo "  Wine:  $(wine --version 2>&1 | head -1)"

echo ""
echo "════════════════════════════════════════════════════════"
echo "  ✅ Build environment ready!"
echo "  Next: bash tools/linux/setup-sdl2-mingw.sh"
echo "       then: bash tools/linux/build.sh"
echo "════════════════════════════════════════════════════════"

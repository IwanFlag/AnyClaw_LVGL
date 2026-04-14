#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  install-mingw.sh — 安装 mingw-w64 + zip（Ubuntu 24.04 Noble）
#
#  用法: bash build/linux/mingw-debs/install-mingw.sh
#
#  用途: 当 apt 源不可达时，用本地包安装交叉编译工具链
# ═══════════════════════════════════════════════════════════════
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export PATH="/usr/local/sbin:/usr/sbin:/sbin:$PATH"

# 策略1: 尝试 apt 安装（最快）
echo "[1/3] 尝试 apt 安装 ..."
if apt-get install -y mingw-w64 cmake zip pkg-config 2>/dev/null; then
    echo "  ✅ apt 安装成功"
else
    echo "  ⚠️  apt 不可用，使用本地包 ..."
    
    # 策略2: 从 tar.gz 解压安装
    if [ -f "$SCRIPT_DIR/mingw-gcc-debs.tar.gz" ]; then
        echo "[2/3] 从 mingw-gcc-debs.tar.gz 解压安装 ..."
        TMPDIR=$(mktemp -d)
        tar xzf "$SCRIPT_DIR/mingw-gcc-debs.tar.gz" -C "$TMPDIR"
        dpkg -i "$TMPDIR"/*.deb 2>&1 || true
        rm -rf "$TMPDIR"
    fi
    
    # 策略3: 安装目录下散落的 .deb
    DEBS=$(ls "$SCRIPT_DIR"/*.deb 2>/dev/null | grep -v mingw-gcc)
    if [ -n "$DEBS" ]; then
        echo "[2b/3] 安装剩余 .deb ..."
        dpkg -i $DEBS 2>&1 || true
    fi
fi

echo "[3/3] 修复依赖 ..."
apt-get -f install -y 2>/dev/null || true

echo "---"
x86_64-w64-mingw32-gcc --version 2>/dev/null | head -1 && echo "✅ gcc-mingw OK" || echo "❌ gcc-mingw 缺失"
x86_64-w64-mingw32-g++ --version 2>/dev/null | head -1 && echo "✅ g++-mingw OK" || echo "❌ g++-mingw 缺失"
which cmake 2>/dev/null && echo "✅ cmake OK" || echo "❌ cmake 缺失"
which zip 2>/dev/null && echo "✅ zip OK" || echo "❌ zip 缺失"
which pkg-config 2>/dev/null && echo "✅ pkg-config OK" || echo "⚠️  pkg-config 缺失（非必须）"

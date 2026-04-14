#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  install-mingw.sh — 从本地 .deb 安装 mingw-w64 + zip（Ubuntu 24.04 Noble）
#
#  用法: bash build/linux/mingw-debs/install-mingw.sh
#
#  用途: 当 apt 源不可达时，用本地包安装交叉编译工具链
# ═══════════════════════════════════════════════════════════════
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export PATH="/usr/local/sbin:/usr/sbin:/sbin:$PATH"

echo "[1/2] 安装 mingw-w64 + zip ..."
dpkg -i "$SCRIPT_DIR"/*.deb 2>&1 || true

echo "[2/2] 修复依赖 ..."
apt-get -f install -y 2>&1 | tail -3

echo "---"
x86_64-w64-mingw32-gcc --version 2>/dev/null | head -1 && echo "✅ mingw-w64 安装成功" || echo "❌ mingw-w64 安装失败"
which zip && echo "✅ zip 安装成功" || echo "❌ zip 安装失败"

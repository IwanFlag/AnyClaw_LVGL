#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  install-wine11.sh — 从本地 .deb 安装 Wine 11 (Ubuntu 24.04 Noble)
#
#  用法: bash build/linux/wine11/install-wine11.sh
#
#  前置条件: mingw-w64 等 i386 基础依赖已通过 apt 安装
#  如果阿里云镜像不通，先切源:
#    cp build/linux/wine11/ustc-sources.list /etc/apt/sources.list.d/
#    apt-get update
# ═══════════════════════════════════════════════════════════════
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export PATH="/usr/local/sbin:/usr/sbin:/sbin:$PATH"

echo "[1/4] 添加 i386 架构..."
dpkg --add-architecture i386 2>/dev/null || true

echo "[2/4] 安装 WineHQ GPG Key + 仓库源..."
mkdir -pm755 /etc/apt/keyrings
cp "$SCRIPT_DIR/winehq-archive.key" /etc/apt/keyrings/
cp "$SCRIPT_DIR/winehq-noble.sources" /etc/apt/sources.list.d/

echo "[3/4] 从本地 .deb 安装 Wine 11..."
dpkg -i "$SCRIPT_DIR"/*.deb 2>&1 || true

echo "[4/4] 修复依赖..."
apt-get -f install -y 2>&1 | tail -3

echo "---"
wine --version && echo "✅ Wine 安装成功" || echo "❌ Wine 安装失败"

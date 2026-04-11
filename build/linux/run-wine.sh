#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  AnyClaw LVGL — Wine 运行测试 (headless with Xvfb)
#  用法: bash build/linux/run-wine.sh [build_dir] [timeout_seconds]
#  示例: bash build/linux/run-wine.sh out/mingw 10
# ═══════════════════════════════════════════════════════════════

PROJECT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_DIR="${1:-${PROJECT_DIR}/out/mingw}"
TIMEOUT="${2:-10}"
BIN_DIR="${BUILD_DIR}/bin"
DISPLAY_NUM=99

echo "════════════════════════════════════════════════════════"
echo "  AnyClaw LVGL — Wine Test Run"
echo "  Binary: ${BIN_DIR}/AnyClaw_LVGL.exe"
echo "  Timeout: ${TIMEOUT}s"
echo "════════════════════════════════════════════════════════"

# 检查二进制是否存在
if [ ! -f "${BIN_DIR}/AnyClaw_LVGL.exe" ]; then
    echo "  ❌ Binary not found. Run build.sh first."
    exit 1
fi

# 清理旧的 Wine 进程
pkill -f AnyClaw_LVGL 2>/dev/null || true
sleep 0.5

# 启动 Xvfb（如果没有运行）
if ! xdpyinfo -display :${DISPLAY_NUM} >/dev/null 2>&1; then
    echo "[1/3] Starting Xvfb on :${DISPLAY_NUM}..."
    Xvfb :${DISPLAY_NUM} -screen 0 1920x1080x24 &
    XVFB_PID=$!
    sleep 1
    echo "  ✓ Xvfb started (PID: ${XVFB_PID})"
else
    echo "[1/3] Xvfb already running on :${DISPLAY_NUM}"
    XVFB_PID=""
fi

export DISPLAY=:${DISPLAY_NUM}
export WINEPREFIX="${HOME}/.wine"

# 初始化 Wine prefix（如果不存在）
if [ ! -d "${WINEPREFIX}" ]; then
    echo "[2/3] Initializing Wine prefix..."
    wineboot --init 2>/dev/null
    echo "  ✓ Wine prefix initialized"
else
    echo "[2/3] Wine prefix exists"
fi

# 运行
echo "[3/3] Running AnyClaw_LVGL.exe for ${TIMEOUT}s..."
cd "${BIN_DIR}"
LOG_FILE="/tmp/anyclaw-wine-test-$$.log"
timeout ${TIMEOUT} wine AnyClaw_LVGL.exe 2>&1 | tee "${LOG_FILE}" || true

# 分析结果
echo ""
echo "════════════════════════════════════════════════════════"
if grep -q "\[MAIN\].*AnyClaw LVGL" "${LOG_FILE}"; then
    echo "  ✅ Application started successfully"
    # 检查是否有实际崩溃（排除 "[CRASH] Crash handler installed" 误报）
    REAL_CRASH=$(grep -c "\[CRASH\]" "${LOG_FILE}" 2>/dev/null || echo 0)
    HANDLER_LINES=$(grep -c "Crash handler installed" "${LOG_FILE}" 2>/dev/null || echo 0)
    if [ "$REAL_CRASH" -gt "$HANDLER_LINES" ]; then
        echo "  ⚠️  Application crashed after startup"
    fi
elif grep -q "CRASH" "${LOG_FILE}" && ! grep -q "Crash handler installed" "${LOG_FILE}"; then
    echo "  ⚠️  Application crashed during startup"
else
    echo "  ⚠️  No clear success message found"
fi

# 统计日志
ERRORS=$(grep -c "ERROR\|CRASH" "${LOG_FILE}" 2>/dev/null || echo 0)
WARNS=$(grep -c "WARN" "${LOG_FILE}" 2>/dev/null || echo 0)
echo "  Errors: ${ERRORS}, Warnings: ${WARNS}"
echo "  Full log: ${LOG_FILE}"

# 清理
pkill -f AnyClaw_LVGL 2>/dev/null || true
if [ -n "$XVFB_PID" ]; then
    kill $XVFB_PID 2>/dev/null || true
fi

echo "════════════════════════════════════════════════════════"

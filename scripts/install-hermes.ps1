# ============================================================
# AnyClaw × Hermes Agent 一键安装器 (Windows PowerShell)
# 用法: 右键 → 以管理员身份运行 PowerShell → 执行此脚本
# ============================================================
#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"

function Write-Info  { param($msg) Write-Host "[INFO] $msg" -ForegroundColor Cyan }
function Write-Ok    { param($msg) Write-Host "[✓] $msg" -ForegroundColor Green }
function Write-Warn  { param($msg) Write-Host "[!] $msg" -ForegroundColor Yellow }
function Write-Fail  { param($msg) Write-Host "[✗] $msg" -ForegroundColor Red }

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  🧄🦞 AnyClaw × Hermes Agent 安装器" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# ---- 1. 检查/安装 WSL2 ----
Write-Info "检查 WSL2 状态..."

$wslStatus = wsl --status 2>&1 | Out-String
if ($wslStatus -match "WSL 2") {
    Write-Ok "WSL2 已安装"
} else {
    Write-Info "正在安装 WSL2..."
    wsl --install --no-launch
    Write-Warn "WSL2 安装完成，需要重启电脑"
    Write-Warn "重启后请重新运行此脚本"
    $restart = Read-Host "是否立即重启? (y/N)"
    if ($restart -eq 'y') {
        Restart-Computer
    }
    exit 0
}

# 检查 WSL 发行版
$distros = wsl --list --quiet 2>&1
if (-not ($distros -match "Ubuntu")) {
    Write-Info "安装 Ubuntu 发行版..."
    wsl --install -d Ubuntu --no-launch
    Write-Ok "Ubuntu 已安装"
}

# ---- 2. 准备安装脚本 ----
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$bashScript = Join-Path $scriptDir "install-hermes.sh"

if (-not (Test-Path $bashScript)) {
    Write-Fail "找不到 install-hermes.sh，请确保两个脚本在同一目录"
    exit 1
}

# 将脚本复制到 WSL 可访问的位置
$wslScriptPath = "/tmp/install-hermes.sh"
$winTempPath = "\\wsl$\Ubuntu\tmp\install-hermes.sh"

Write-Info "准备 WSL 安装环境..."
wsl -e bash -c "cp /mnt/`$(wslpath -w '$bashScript') /tmp/install-hermes.sh 2>/dev/null || true"

# 备用方案：直接写入
if (-not (wsl -e test -f $wslScriptPath 2>$null)) {
    Write-Info "通过管道传输安装脚本..."
    $scriptContent = Get-Content $bashScript -Raw
    $scriptContent | wsl -e bash -c "cat > /tmp/install-hermes.sh && chmod +x /tmp/install-hermes.sh"
}

Write-Ok "安装脚本已就绪"

# ---- 3. 在 WSL 中执行安装 ----
Write-Info "在 WSL 中启动安装（需要输入 WSL 密码）..."
Write-Host ""

wsl -e bash -c "cd /tmp && bash install-hermes.sh"

$exitCode = $LASTEXITCODE

Write-Host ""
if ($exitCode -eq 0) {
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "  ✅ 安装完成！" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "  下一步：" -ForegroundColor Yellow
    Write-Host "  1. 打开 AnyClaw → Settings → Runtime → 选择 Hermes Agent"
    Write-Host "  2. （可选）在 WSL 运行 hermes setup 配置 API Key"
    Write-Host "  3. （可选）在 WSL 运行 hermes gateway setup 接入 IM"
    Write-Host ""
    Write-Host "  打开 WSL 终端:  wsl" -ForegroundColor Cyan
    Write-Host "  检查状态:       wsl -e hermes gateway status" -ForegroundColor Cyan
    Write-Host ""
} else {
    Write-Fail "安装过程中出现错误 (exit code: $exitCode)"
    Write-Warn "请在 WSL 中手动运行: bash /tmp/install-hermes.sh"
}

Read-Host "按 Enter 退出"

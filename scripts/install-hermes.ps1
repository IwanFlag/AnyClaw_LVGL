# ============================================================
# AnyClaw × Hermes Agent 一键安装器 (Windows PowerShell)
# 用法: 右键 → 以管理员身份运行 PowerShell → 执行此脚本
# ============================================================
#Requires -RunAsAdministrator

$ErrorActionPreference = "Continue"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

function Write-Info  { param($msg) Write-Host "[INFO] $msg" -ForegroundColor Cyan }
function Write-Ok    { param($msg) Write-Host "[✓]   $msg" -ForegroundColor Green }
function Write-Warn  { param($msg) Write-Host "[!]   $msg" -ForegroundColor Yellow }
function Write-Fail  { param($msg) Write-Host "[✗]   $msg" -ForegroundColor Red }

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  🧄🦞 AnyClaw × Hermes Agent 安装器" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# ---- 1. 检查/安装 WSL2 ----
Write-Info "检查 WSL2 状态..."

# 检查 wsl 命令是否存在
$wslExists = $null -ne (Get-Command wsl -ErrorAction SilentlyContinue)

if (-not $wslExists) {
    Write-Warn "WSL 未安装，正在安装..."
    Write-Info "这可能需要几分钟，请耐心等待..."
    wsl --install --no-launch 2>&1 | Out-Null
    Write-Warn "============================================"
    Write-Warn "  WSL 安装需要重启电脑才能生效！"
    Write-Warn "  重启后请重新运行此脚本"
    Write-Warn "============================================"
    $restart = Read-Host "是否立即重启? (y/N)"
    if ($restart -eq 'y' -or $restart -eq 'Y') {
        Restart-Computer
    }
    exit 0
}

# 检查 WSL 是否可用（通过尝试简单命令）
$wslTest = $null
try {
    $wslTest = wsl echo "ok" 2>&1
} catch {
    $wslTest = $null
}

if ($wslTest -match "ok" -or $wslTest -match "WSL") {
    # WSL 可用，但可能需要启用虚拟机平台
    $wslVer = $null
    try {
        $wslVer = wsl --version 2>&1 | Select-String "WSL" | Select-Object -First 1
    } catch {}
    
    if ($wslVer) {
        Write-Ok "WSL2 已安装: $($wslVer.ToString().Trim())"
    } else {
        Write-Ok "WSL 已安装"
    }
} else {
    # WSL 存在但不可用 — 可能需要启用功能
    Write-Warn "WSL 命令存在但无法运行，可能需要启用 Windows 功能"
    Write-Info "正在尝试启用所需功能..."
    
    # 启用必要功能
    $features = @("Microsoft-Windows-Subsystem-Linux", "VirtualMachinePlatform")
    foreach ($f in $features) {
        $state = (Get-WindowsOptionalFeature -Online -FeatureName $f -ErrorAction SilentlyContinue).State
        if ($state -ne "Enabled") {
            Write-Info "启用 $f ..."
            Enable-WindowsOptionalFeature -Online -FeatureName $f -NoRestart -All 2>&1 | Out-Null
        } else {
            Write-Ok "$f 已启用"
        }
    }
    
    Write-Warn "============================================"
    Write-Warn "  需要重启电脑才能使 WSL 生效！"
    Write-Warn "  重启后请重新运行此脚本"
    Write-Warn "============================================"
    $restart = Read-Host "是否立即重启? (y/N)"
    if ($restart -eq 'y' -or $restart -eq 'Y') {
        Restart-Computer
    }
    exit 0
}

# 检查 WSL 发行版
$distros = $null
try {
    $distros = wsl --list --quiet 2>&1
} catch {}

if (-not ($distros -match "Ubuntu")) {
    Write-Info "安装 Ubuntu 发行版（首次运行可能需要几分钟）..."
    Write-Warn "安装过程中请按提示设置 Linux 用户名和密码"
    wsl --install -d Ubuntu 2>&1 | Out-Null
    
    # 等待安装完成
    Write-Info "等待 Ubuntu 初始化..."
    $maxWait = 120
    $waited = 0
    while ($waited -lt $maxWait) {
        Start-Sleep -Seconds 5
        $waited += 5
        $check = wsl -d Ubuntu echo "ready" 2>&1
        if ($check -match "ready") {
            Write-Ok "Ubuntu 安装完成"
            break
        }
        Write-Host "." -NoNewline
    }
    Write-Host ""
    
    if ($waited -ge $maxWait) {
        Write-Warn "Ubuntu 安装可能仍在进行，请稍后手动验证"
    }
} else {
    Write-Ok "Ubuntu 发行版已安装"
}

# ---- 2. 准备安装脚本 ----
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$bashScript = Join-Path $scriptDir "install-hermes.sh"

if (-not (Test-Path $bashScript)) {
    Write-Fail "找不到 install-hermes.sh"
    Write-Warn "请确保 install-hermes.sh 和 install-hermes.ps1 在同一目录下"
    Write-Warn "当前目录: $scriptDir"
    Read-Host "按 Enter 退出"
    exit 1
}

Write-Info "传输安装脚本到 WSL..."

# 方法1: 通过 /mnt 路径复制
$winPath = $bashScript -replace '\\', '/'
$driveLetter = $winPath.Substring(0,1).ToLower()
$wslPath = "/mnt/$driveLetter" + $winPath.Substring(2)

$copyResult = wsl -e bash -c "cp '$wslPath' /tmp/install-hermes.sh && chmod +x /tmp/install-hermes.sh && echo OK" 2>&1

if ($copyResult -match "OK") {
    Write-Ok "脚本传输完成"
} else {
    # 方法2: 通过管道写入
    Write-Info "通过管道传输..."
    $scriptContent = Get-Content $bashScript -Raw -Encoding UTF8
    $scriptContent | wsl -e bash -c "cat > /tmp/install-hermes.sh && chmod +x /tmp/install-hermes.sh && echo OK" 2>&1 | Out-Null
    Write-Ok "脚本传输完成"
}

# ---- 3. 在 WSL 中执行安装 ----
Write-Host ""
Write-Info "在 WSL 中启动安装..."
Write-Warn "接下来需要输入 WSL 密码（安装依赖要用 sudo）"
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

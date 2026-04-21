# ============================================================
# AnyClaw x Hermes Agent Installer (Windows PowerShell)
# 官方安装器 + MiMo-V2 配置引导
#
# 运行方式（二选一）:
#   方式1: 允许脚本执行后运行
#     Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
#     .\install-hermes.ps1
#
#   方式2: 每次用 Bypass 启动
#     powershell -ExecutionPolicy Bypass -File .\install-hermes.ps1
# ============================================================
#Requires -RunAsAdministrator

$ErrorActionPreference = "Continue"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

function Info  { param($m) Write-Host "[INFO] $m" -ForegroundColor Cyan }
function Ok    { param($m) Write-Host "[OK]   $m" -ForegroundColor Green }
function Warn  { param($m) Write-Host "[!]    $m" -ForegroundColor Yellow }
function Fail  { param($m) Write-Host "[ERR]  $m" -ForegroundColor Red }

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  🧄🦞 AnyClaw x Hermes Agent Installer" -ForegroundColor Cyan
Write-Host "  Windows Edition" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# ---- 0. 杀毒软件排除（避免 uv 安装依赖时报错 os error -2147024786） ----
Info "添加 Windows Defender 排除目录（避免安装时文件被锁定）..."
try {
    Add-MpPreference -ExclusionPath "$env:TEMP" -ErrorAction SilentlyContinue
    Add-MpPreference -ExclusionPath "$env:LOCALAPPDATA\hermes" -ErrorAction SilentlyContinue
    Ok "排除项已添加（TEMP + hermes 目录）"
} catch {
    Warn "Add-MpPreference 失败，可能是第三方杀软"
    Warn "如安装报错，请手动将以下目录加入杀软白名单："
    Warn "  $env:TEMP"
    Warn "  $env:LOCALAPPDATA\hermes"
}

# ---- 1. 检查是否已安装 ----
$HERMES_HOME = "$env:LOCALAPPDATA\hermes"
$INSTALL_DIR = "$HERMES_HOME\hermes-agent"
$hermesExe = "$INSTALL_DIR\venv\Scripts\hermes.exe"

if (Test-Path $hermesExe) {
    $ver = & $hermesExe --version 2>$null
    Ok "Hermes Agent 已安装 ($ver)，跳过安装步骤"
    $skipInstall = $true
} else {
    $skipInstall = $false
}

# ---- 2. 运行官方安装脚本（未安装时） ----
if (-not $skipInstall) {
    Info "运行 Hermes Agent 官方安装脚本..."

    $officialInstallUrl = "https://raw.githubusercontent.com/NousResearch/hermes-agent/main/scripts/install.ps1"

    try {
        irm $officialInstallUrl | iex
    } catch {
        Fail "官方安装脚本执行失败: $_"
        Write-Host ""
        Warn "请手动运行:"
        Write-Host "  irm $officialInstallUrl | iex" -ForegroundColor Yellow
        exit 1
    }
} else {
    # 已安装时尝试更新（HTTPS 方式，避免 SSH 密钥问题）
    Info "尝试更新到最新版..."
    if (Test-Path "$INSTALL_DIR\.git") {
        Push-Location $INSTALL_DIR
        # 强制用 HTTPS，避免 SSH 密钥弹密码
        git remote set-url origin https://github.com/NousResearch/hermes-agent.git 2>$null
        git -c windows.appendAtomically=false fetch origin --quiet 2>$null
        git -c windows.appendAtomically=false checkout main --quiet 2>$null
        git -c windows.appendAtomically=false pull origin main --quiet 2>$null
        $pullExit = $LASTEXITCODE
        Pop-Location
        if ($pullExit -eq 0) {
            Ok "代码已更新到最新版"
        } else {
            Warn "更新失败（不影响已有安装），可手动运行: hermes update"
        }
    }

    # 更新依赖
    Info "更新 Python 依赖..."
    Push-Location $INSTALL_DIR
    try {
        & "$INSTALL_DIR\venv\Scripts\python.exe" -m pip install -e ".[all]" --quiet 2>$null
        Ok "依赖已更新"
    } catch {
        Warn "依赖更新失败，可手动运行: cd $INSTALL_DIR && uv pip install -e ."
    }
    Pop-Location
}

# ---- 3. MiMo-V2 模型配置 ----
Write-Host ""
Info "配置 Xiaomi MiMo-V2..."

$configPath = "$HERMES_HOME\config.yaml"

# 确保 config.yaml 存在（首次安装时官方脚本会创建）
if (-not (Test-Path $configPath)) {
    $examplePath = "$INSTALL_DIR\cli-config.yaml.example"
    if (Test-Path $examplePath) {
        Copy-Item $examplePath $configPath
        Ok "从模板创建 config.yaml"
    } else {
        New-Item -ItemType File -Path $configPath -Force | Out-Null
        Ok "创建空 config.yaml"
    }
}

$configContent = Get-Content $configPath -Raw -ErrorAction SilentlyContinue
if ($configContent -notmatch "mimo-v2") {
    $mimoConfig = @"

# ---- Xiaomi MiMo-V2 (Nous Portal) ----
# 限免：4月8日-4月22日 24:00 (UTC+8)
# 通过 Nous Portal OAuth 免费调用，不需要 API Key，不需要充值
model:
  primary: "xiaomi/mimo-v2-pro"
  fallback: "xiaomi/mimo-v2-flash"
  available:
    - id: "xiaomi/mimo-v2-pro"
      name: "MiMo-V2 Pro"
      description: "推理旗舰，1M 长上下文，深度 Agent 优化"
      context_window: 1048576
      capabilities: [chat, tools, reasoning]
    - id: "xiaomi/mimo-v2-omni"
      name: "MiMo-V2 Omni"
      description: "全模态理解，图像/视频/音频+文本"
      context_window: 1048576
      capabilities: [chat, tools, reasoning, vision, audio, video]
    - id: "xiaomi/mimo-v2-flash"
      name: "MiMo-V2 Flash"
      description: "极速响应，轻量任务首选"
      context_window: 262144
      capabilities: [chat, tools]
"@
    Add-Content -Path $configPath -Value $mimoConfig -Encoding UTF8
    Ok "MiMo-V2 模型配置已写入 config.yaml"
} else {
    Ok "config.yaml 已包含 MiMo-V2 配置"
}

# ---- 4. 完成 ----
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  ✅ 安装完成！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "  默认模型:     " -NoNewline; Write-Host "xiaomi/mimo-v2-pro" -ForegroundColor Cyan
Write-Host "  可用模型:     " -NoNewline; Write-Host "Pro / Omni / Flash" -ForegroundColor Cyan
Write-Host "  配置目录:     " -NoNewline; Write-Host "$HERMES_HOME" -ForegroundColor Cyan
Write-Host ""
Write-Host "  📌 MiMo-V2 限免（4月8日-22日）通过 Nous Portal OAuth 免费调用" -ForegroundColor Green
Write-Host "     不需要 API Key，不需要充值" -ForegroundColor Green
Write-Host ""
Write-Host "  下一步:" -ForegroundColor Yellow
Write-Host "  1. 重启终端使 PATH 生效（首次安装时）"
Write-Host "  2. " -NoNewline; Write-Host "hermes setup" -ForegroundColor Cyan -NoNewline; Write-Host " — 选 Nous Portal OAuth 登录"
Write-Host "  3. " -NoNewline; Write-Host "hermes" -ForegroundColor Cyan -NoNewline; Write-Host " — 开始对话"
Write-Host "  4. " -NoNewline; Write-Host "hermes gateway setup" -ForegroundColor Cyan -NoNewline; Write-Host " — 接入 Telegram / Discord"
Write-Host ""
Write-Host "  常用命令:" -ForegroundColor Yellow
Write-Host "  hermes setup        # 配置向导"
Write-Host "  hermes model        # 切换模型"
Write-Host "  hermes gateway      # 启动消息网关"
Write-Host "  hermes update       # 更新到最新版"
Write-Host ""

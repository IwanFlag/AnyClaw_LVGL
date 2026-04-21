# ============================================================
# AnyClaw x Claude CLI Installer (Windows PowerShell)
# Downloads Claude CLI MSI and performs silent install
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File download-claude.ps1
# ============================================================
$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$CLAUDE_VERSION = "latest"
$DOWNLOAD_DIR   = "$env:TEMP\anyclaw-claude"
$MSI_PATH       = "$DOWNLOAD_DIR\claude-enterprise-windows-amd64.msi"
$INSTALL_LOG   = "$DOWNLOAD_DIR\install.log"

function Info  { param($m) Write-Host "[INFO] $m" -ForegroundColor Cyan }
function Ok    { param($m) Write-Host "[OK]   $m" -ForegroundColor Green }
function Warn  { param($m) Write-Host "[!]    $m" -ForegroundColor Yellow }
function Fail  { param($m) Write-Host "[ERR]  $m" -ForegroundColor Red; exit 1 }

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Claude CLI Installer for AnyClaw" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# ---- 0. 检测是否已安装 ----
$claudePath = $null
$searchPaths = @(
    "$env:LOCALAPPDATA\Programs\Claude\claude.exe",
    "$env:ProgramFiles\Claude\claude.exe",
    "$env:ProgramFiles(x86)\Claude\claude.exe",
    "$env:USERPROFILE\.claude\bin\claude.exe"
)
foreach ($p in $searchPaths) {
    if (Test-Path $p) { $claudePath = $p; break }
}
if ($claudePath) {
    try {
        $ver = & $claudePath --version 2>$null
        Ok "Claude CLI 已安装 ($ver)"
        Info "跳过安装。如需重新安装，请先卸载。"
        exit 0
    } catch { }
}

# ---- 1. 创建临时目录 ----
if (-not (Test-Path $DOWNLOAD_DIR)) {
    New-Item -ItemType Directory -Path $DOWNLOAD_DIR -Force | Out-Null
}
Set-Location $DOWNLOAD_DIR

# ---- 2. 获取最新版本下载链接 ----
Info "获取 Claude CLI 最新版本..."
try {
    $releasesJson = Invoke-RestMethod "https://api.github.com/repos/anthropics/claude-code/releases/$CLAUDE_VERSION" -TimeoutSec 15
    $msiAsset = $releasesJson.assets | Where-Object { $_.name -match "windows-amd64\.msi$" } | Select-Object -First 1
    if (-not $msiAsset) {
        Fail "未找到 Windows MSI 资源，请检查官方仓库"
    }
    $downloadUrl = $msiAsset.browser_download_url
    $version = $releasesJson.tag_name -replace '^v', ''
    Info "最新版本: v$version"
} catch {
    Fail "获取版本信息失败: $_"
}

# ---- 3. 下载 MSI ----
$msiName = Split-Path $msiAsset.name -Leaf
$MSI_PATH = "$DOWNLOAD_DIR\$msiName"
Info "下载: $downloadUrl"
try {
    $wc = New-Object System.Net.WebClient
    $wc.DownloadFile($downloadUrl, $MSI_PATH)
    $wc.Dispose()
    $sizeMB = [math]::Round((Get-Item $MSI_PATH).Length / 1MB, 1)
    Ok "下载完成 ($sizeMB MB) → $MSI_PATH"
} catch {
    Fail "下载失败: $_"
}

# ---- 4. 静默安装 MSI ----
Info "开始安装（需要管理员权限）..."
$msiLog = "$DOWNLOAD_DIR\msi_install.log"
try {
    $proc = Start-Process msiexec.exe -ArgumentList "/i `"$MSI_PATH`" /qn /norestart /log `"$msiLog`"" -Wait -PassThru -Verb RunAs
    if ($proc.ExitCode -eq 0) {
        Ok "安装成功"
    } else {
        Warn "MSI 安装返回码: $($proc.ExitCode)"
        if (Test-Path $msiLog) {
            $errLines = Select-String $msiLog -Pattern "Error|Fail" -Quiet
            if ($errLines) { Warn "详见: $msiLog" }
        }
    }
} catch {
    Fail "安装失败（需要管理员权限）: $_"
}

# ---- 5. 验证 ----
$claudePath = "$env:LOCALAPPDATA\Programs\Claude\claude.exe"
if (Test-Path $claudePath) {
    try {
        $ver = & $claudePath --version 2>$null
        Ok "验证通过: $ver"
    } catch {
        Warn "安装完成但验证失败，请手动运行: claude --version"
    }
} else {
    Warn "未找到安装路径，尝试其他位置..."
    foreach ($p in $searchPaths) {
        if (Test-Path $p) {
            try {
                $ver = & $p --version 2>$null
                Ok "验证通过: $p → $ver"
                break
            } catch { }
        }
    }
}

# ---- 6. 清理临时文件 ----
Info "清理临时文件..."
Remove-Item $DOWNLOAD_DIR -Recurse -Force -ErrorAction SilentlyContinue
Ok "完成"
Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  下一步:" -ForegroundColor Yellow
Write-Host "  1. 重启 AnyClaw" -ForegroundColor White
Write-Host "  2. 在设置中配置 API Key" -ForegroundColor White
Write-Host "========================================" -ForegroundColor Green

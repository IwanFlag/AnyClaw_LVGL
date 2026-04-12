# ============================================================
# AnyClaw x Hermes Agent Installer (Windows PowerShell)
# Usage: Right-click -> Run as Administrator
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
Write-Host "  AnyClaw x Hermes Agent Installer" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# ---- 1. Check / Install WSL2 ----
Info "Checking WSL2..."

$wslExists = $null -ne (Get-Command wsl -ErrorAction SilentlyContinue)

if (-not $wslExists) {
    Warn "WSL not installed. Installing..."
    Info "This may take a few minutes..."
    wsl --install --no-launch 2>&1 | Out-Null
    Write-Host ""
    Warn "============================================"
    Warn "  WSL installed! Restart required."
    Warn "  After restart, run this script again."
    Warn "============================================"
    $restart = Read-Host "Restart now? (y/N)"
    if ($restart -eq 'y' -or $restart -eq 'Y') {
        Restart-Computer
    }
    exit 0
}

# Test if WSL actually works
$wslTest = $null
try { $wslTest = wsl echo "ok" 2>&1 } catch { $wslTest = $null }

if ($wslTest -match "ok") {
    Ok "WSL is working"
} else {
    Warn "WSL command exists but not functional. Enabling required features..."
    
    $features = @("Microsoft-Windows-Subsystem-Linux", "VirtualMachinePlatform")
    foreach ($f in $features) {
        $state = (Get-WindowsOptionalFeature -Online -FeatureName $f -ErrorAction SilentlyContinue).State
        if ($state -ne "Enabled") {
            Info "Enabling $f ..."
            Enable-WindowsOptionalFeature -Online -FeatureName $f -NoRestart -All 2>&1 | Out-Null
        } else {
            Ok "$f already enabled"
        }
    }
    
    Write-Host ""
    Warn "============================================"
    Warn "  Restart required to activate WSL!"
    Warn "  After restart, run this script again."
    Warn "============================================"
    $restart = Read-Host "Restart now? (y/N)"
    if ($restart -eq 'y' -or $restart -eq 'Y') {
        Restart-Computer
    }
    exit 0
}

# Check Ubuntu distro
$distros = $null
try { $distros = wsl --list --quiet 2>&1 } catch {}

if (-not ($distros -match "Ubuntu")) {
    Info "Installing Ubuntu (may take a few minutes)..."
    Warn "You will be asked to set a Linux username and password"
    wsl --install -d Ubuntu 2>&1 | Out-Null
    
    Info "Waiting for Ubuntu to initialize..."
    $maxWait = 120
    $waited = 0
    while ($waited -lt $maxWait) {
        Start-Sleep -Seconds 5
        $waited += 5
        $check = wsl -d Ubuntu echo "ready" 2>&1
        if ($check -match "ready") {
            Ok "Ubuntu installed"
            break
        }
        Write-Host "." -NoNewline
    }
    Write-Host ""
    
    if ($waited -ge $maxWait) {
        Warn "Ubuntu install may still be in progress. Verify later."
    }
} else {
    Ok "Ubuntu already installed"
}

# ---- 2. Prepare install script ----
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$bashScript = Join-Path $scriptDir "install-hermes.sh"

if (-not (Test-Path $bashScript)) {
    Fail "install-hermes.sh not found"
    Warn "Make sure install-hermes.sh is in the same folder as this script"
    Warn "Current folder: $scriptDir"
    Read-Host "Press Enter to exit"
    exit 1
}

Info "Copying script to WSL..."

$winPath = $bashScript -replace '\\', '/'
$driveLetter = $winPath.Substring(0,1).ToLower()
$wslPath = "/mnt/$driveLetter" + $winPath.Substring(2)

$copyResult = wsl -e bash -c "cp '$wslPath' /tmp/install-hermes.sh && chmod +x /tmp/install-hermes.sh && echo OK" 2>&1

if ($copyResult -match "OK") {
    Ok "Script copied"
} else {
    Info "Trying pipe method..."
    $scriptContent = Get-Content $bashScript -Raw -Encoding UTF8
    $scriptContent | wsl -e bash -c "cat > /tmp/install-hermes.sh && chmod +x /tmp/install-hermes.sh && echo OK" 2>&1 | Out-Null
    Ok "Script copied"
}

# ---- 3. Run installation in WSL ----
Write-Host ""
Info "Starting installation in WSL..."
Warn "You will need to enter your WSL password for sudo"
Write-Host ""

wsl -e bash -c "cd /tmp && bash install-hermes.sh"

$exitCode = $LASTEXITCODE

Write-Host ""
if ($exitCode -eq 0) {
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "  [OK] Installation complete!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "  Next steps:" -ForegroundColor Yellow
    Write-Host "  1. Open AnyClaw -> Settings -> Runtime -> select Hermes Agent"
    Write-Host "  2. (Optional) In WSL: hermes setup (configure API Key)"
    Write-Host "  3. (Optional) In WSL: hermes gateway setup (connect IM)"
    Write-Host ""
    Write-Host "  Open WSL terminal:  wsl" -ForegroundColor Cyan
    Write-Host "  Check status:       wsl -e hermes gateway status" -ForegroundColor Cyan
    Write-Host ""
} else {
    Fail "Installation failed (exit code: $exitCode)"
    Warn "Please run manually in WSL: bash /tmp/install-hermes.sh"
}

Read-Host "Press Enter to exit"

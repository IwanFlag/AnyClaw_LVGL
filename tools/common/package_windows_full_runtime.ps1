param(
    [Parameter(Mandatory = $true)][string]$ProjectDir,
    [Parameter(Mandatory = $true)][string]$ExePath,
    [Parameter(Mandatory = $true)][string]$SdlPath,
    [Parameter(Mandatory = $true)][string]$OutRoot
)

$ErrorActionPreference = "Stop"

if (!(Test-Path $ProjectDir)) { throw "ProjectDir not found: $ProjectDir" }
if (!(Test-Path $ExePath)) { throw "Exe not found: $ExePath" }
if (!(Test-Path $SdlPath)) { throw "SDL2.dll not found: $SdlPath" }

New-Item -ItemType Directory -Force -Path $OutRoot | Out-Null
$ts = Get-Date -Format "yyyyMMdd_HHmmss"
$pkgDir = Join-Path $OutRoot ("AnyClaw_LVGL_windows_full-runtime_" + $ts)
New-Item -ItemType Directory -Force -Path $pkgDir | Out-Null

Copy-Item $ExePath $pkgDir -Force
Copy-Item $SdlPath $pkgDir -Force
Copy-Item (Join-Path $ProjectDir "assets") (Join-Path $pkgDir "assets") -Recurse -Force
Copy-Item (Join-Path $ProjectDir "bundled") (Join-Path $pkgDir "bundled") -Recurse -Force

$zipPath = $pkgDir + ".zip"
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path (Join-Path $pkgDir "*") -DestinationPath $zipPath -Force
Write-Output $zipPath

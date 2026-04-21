$ErrorActionPreference = "Continue"
$VsDevCmd = "C:\VSBuildTools\Common7\Tools\VsDevCmd.bat"
$SrcDir = "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL"
$IncLVGL = "$SrcDir\thirdparty\lvgl\src"
$IncBuild = "$SrcDir\build\windows\out"
$IncRoot = "$SrcDir"

$env:Path = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\include;C:\VSBuildTools\WinDir\Include\10.0.22621.0\ucrt;C:\VSBuildTools\WinDir\Include\10.0.22621.0\shared;C:\VSBuildTools\WinDir\Include\10.0.22621.0\um;C:\VSBuildTools\Common7\Tools\WinNT;C:\VSBuildTools\Common7\IDE;C:\Windows\System32;$env:Path"

$cl = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
if (!(Test-Path $cl)) {
    Write-Host "[ERROR] cl.exe not found at: $cl"
    exit 1
}

Write-Host "[INFO] Found cl.exe: $cl"
Write-Host "[INFO] Starting compilation of ui_main.cpp..."

$output = & $cl /nologo /c /EHsc /utf-8 /std:c++17 /DNOMINMAX /I. /I"$IncLVGL" /I"$IncBuild" /I"$IncRoot" /I"$SrcDir\thirdparty\sdl2-windows\include" /I"C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\include" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um" /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt" /Dlvgl=1 "$SrcDir\src\ui_main.cpp" /Fo:"$SrcDir\ui_main.obj" 2>&1

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Compilation failed with exit code: $LASTEXITCODE"
    $output | Select-Object -First 40
} else {
    Write-Host "[SUCCESS] Compilation passed!"
}

# Show context around line 12178 if there's an error
$errLine = $output | Select-String "12178"
if ($errLine) {
    Write-Host "`n[ERROR at line 12178 context:]"
    $output | Select-String "12178" | Select-Object -First 5
}

# AnyClaw LVGL - Build with NMake via cmake --build (avoids PATH issues)
$ErrorActionPreference = "Stop"
$Start = Get-Date

$SrcDir = "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL"
$BuildDir = "$SrcDir\build"

Remove-Item $BuildDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null

Set-Location $SrcDir

# Configure via cmake --build (uses cmake's own environment)
& "C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 > $null 2>&1

$cmakeBat = @"
@echo off
call "C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=%PATH%;C:\Program Files\CMake\bin
"C:\Program Files\CMake\bin\cmake.exe" -G "NMake Makefiles" -B "$BuildDir" -DCMAKE_BUILD_TYPE=Release "$SrcDir"
"@

$cmakeBatPath = "$BuildDir\cmake_config.bat"
[System.IO.File]::WriteAllText($cmakeBatPath, $cmakeBat, [System.Text.Encoding]::UTF8)
cmd /c $cmakeBatPath > "$BuildDir\cmake_config.log" 2>&1
$configureExit = $LASTEXITCODE

if ($configureExit -ne 0) {
    Write-Host "[ERROR] CMake configure failed (exit=$configureExit)"
    if (Test-Path "$BuildDir\cmake_config.log") { Get-Content "$BuildDir\cmake_config.log" -Tail 20 }
    exit $configureExit
}

if ((Test-Path "$BuildDir\cmake_config.log") -and (Select-String "$BuildDir\cmake_config.log" -Pattern "CMake Error" -Quiet)) {
    Write-Host "[ERROR] CMake configure failed"
    Get-Content "$BuildDir\cmake_config.log" -Tail 10
    exit 1
}
Write-Host "[OK] Configure done"

# Build using cmake --build (picks up vcvars environment from cmake_config.bat context)
$buildBat = @"
@echo off
call "C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=%PATH%;C:\Program Files\CMake\bin
"C:\Program Files\CMake\bin\cmake.exe" --build "$BuildDir" 2>&1
"@

$buildBatPath = "$BuildDir\build_run.bat"
[System.IO.File]::WriteAllText($buildBatPath, $buildBat, [System.Text.Encoding]::UTF8)
cmd /c $buildBatPath > "$BuildDir\build.log" 2>&1
$buildExit = $LASTEXITCODE

if ($buildExit -ne 0) {
    Write-Host "[ERROR] Build failed (exit=$buildExit)"
    if (Test-Path "$BuildDir\build.log") { Get-Content "$BuildDir\build.log" -Tail 40 }
    exit $buildExit
}

$Exe = Get-ChildItem "$BuildDir\bin\AnyClaw_LVGL.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if (!$Exe) {
    Write-Host "[ERROR] Build failed - exe not found"
    Get-Content "$BuildDir\build.log" -Tail 10
    exit 1
}

$Elapsed = (Get-Date) - $Start
Write-Host "[OK] Build PASSED in $($Elapsed.TotalSeconds)s"
Write-Host "[OK] Output: $($Exe.FullName)"

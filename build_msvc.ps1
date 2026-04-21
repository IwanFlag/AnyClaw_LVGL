$MsvcRoot = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717"
$SdkRoot = "C:\Program Files (x86)\Windows Kits\10"
$VsBin = "C:\VSBuildTools\VC\Auxiliary\Build"
$SrcDir = "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL"
$BuildDir = "$SrcDir\build"

# Run vcvarsall to set environment, then run cmake
$env:PATH = "$MsvcRoot\bin\Hostx64\x64;$SdkRoot\bin\10.0.22621.0\x64;$env:SystemRoot\System32;$env:SystemRoot"
$env:LIB = "$MsvcRoot\lib\x64;$SdkRoot\Lib\10.0.22621.0\ucrt\x64;$SdkRoot\Lib\10.0.22621.0\um\x64"
$env:INCLUDE = "$MsvcRoot\include;$SdkRoot\Include\10.0.22621.0\ucrt;$SdkRoot\Include\10.0.22621.0\shared;$SdkRoot\Include\10.0.22621.0\um;$SdkRoot\Include\10.0.22621.0\winrt"

Write-Host "[INFO] Cleaning CMake cache..."
Remove-Item "$BuildDir\CMakeCache.txt" -ErrorAction SilentlyContinue

Write-Host "[INFO] Running CMake configure..."
Set-Location $SrcDir
$cmakeExe = "C:\Program Files\CMake\bin\cmake.exe"
& "$VsBin\vcvarsall.bat" x64 > $null 2>&1
& $cmakeExe -G "NMake Makefiles" -B build -DCMAKE_BUILD_TYPE=Release 2>&1 | Select-Object -Last 5

$logFile = "$BuildDir\build.log"
Write-Host "[INFO] Starting build..."
$start = Get-Date
$process = Start-Process -FilePath "nmake" -ArgumentList "/F", "Makefile", "install" -WorkingDirectory $BuildDir -PassThru -NoNewWindow -RedirectStandardOutput $logFile -RedirectStandardError "$BuildDir\build_err.log"
$process.WaitForExit()
$exit = $process.ExitCode
$elapsed = (Get-Date) - $start

Write-Host "[INFO] Build finished in $($elapsed.TotalSeconds)s with exit code: $exit"
if ($exit -ne 0) {
    Write-Host "[ERROR] Build failed. Last 30 lines:"
    Get-Content $logFile -Tail 30 | ForEach-Object { Write-Host $_ }
} else {
    Write-Host "[SUCCESS] Build passed!"
}
exit $exit

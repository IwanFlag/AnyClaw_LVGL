$MsvcBin = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64"
$SdkBin = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64"
$env:Path = "$MsvcBin;$SdkBin;$env:SystemRoot\System32;$env:SystemRoot"
$env:LIB = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64"
$env:INCLUDE = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt"

$SrcDir = "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL"
$BuildDir = "$SrcDir\build"
$LogFile = "$BuildDir\build2.log"

Write-Host "[INFO] Starting build..."
$start = Get-Date
$process = Start-Process -FilePath "C:\Program Files\CMake\bin\cmake.exe" -ArgumentList "--build", "$BuildDir", "--config", "Release" -PassThru -NoNewWindow -RedirectStandardOutput $LogFile -RedirectStandardError "$BuildDir\build_err2.log"
$process.WaitForExit()
$exit = $process.ExitCode
$elapsed = (Get-Date) - $start

Write-Host "[INFO] Build finished in $($elapsed.TotalSeconds)s with exit code: $exit"
if ($exit -ne 0) {
    Write-Host "[ERROR] Build failed. Last 40 lines of log:"
    Get-Content $LogFile -Tail 40 | ForEach-Object { Write-Host $_ }
} else {
    Write-Host "[SUCCESS] Build passed!"
}
exit $exit

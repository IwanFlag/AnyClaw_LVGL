# Stop any running build processes
$exeNames = @("cmake", "nmake", "cl", "link")
foreach ($name in $exeNames) {
    Get-Process -Name $name -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
}
Start-Sleep -Seconds 2
Write-Host "[OK] Build processes stopped"

# Build
$env:Path = "C:\Program Files\CMake\bin;C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64;" + [System.Environment]::GetEnvironmentVariable("Path", "Machine")
$env:LIB = "C:\Program Files (x86)\Windows Kits\10\lib\10.0.22621.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.22621.0\um\x64;C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\lib\x64"
$env:INCLUDE = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt"

$BuildDir = "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\build"
$LogFile = "$BuildDir\build2.log"

Write-Host "[INFO] Starting build..."
$start = Get-Date
$proc = Start-Process -FilePath "cmake" -ArgumentList "--build", $BuildDir, "--config", "Release" -NoNewWindow -PassThru -RedirectStandardOutput $LogFile -RedirectStandardError "$BuildDir\build2_err.log"
Write-Host "[INFO] Build PID: $($proc.Id)"
$proc.WaitForExit()
$exit = $proc.ExitCode
$elapsed = (Get-Date) - $start

if ($exit -ne 0) {
    Write-Host "`n[ERROR] Build FAILED after $($elapsed.TotalMinutes.ToString('0.0')) min"
    Write-Host "Errors:"
    Get-Content $LogFile | Select-String "error C|fatal error" | Select-Object -First 20
    Get-Content "$BuildDir\build2_err.log" | Select-String "error C|fatal error" | Select-Object -First 5
} else {
    Write-Host "`n[SUCCESS] Build PASSED in $($elapsed.TotalMinutes.ToString('0.0')) min"
    Write-Host "Output:"
    Get-Content $LogFile -Tail 5
}
exit $exit

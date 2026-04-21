$ErrorActionPreference = "Continue"
$env:Path = "C:\Program Files\CMake\bin;C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;" + [System.Environment]::GetEnvironmentVariable("Path", "Machine")
$env:LIB = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\lib\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.22621.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.22621.0\um\x64"
$env:INCLUDE = "C:\VSBuildTools\VC\Tools\MSVC\14.50.35717\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\winrt"

$flags = "/DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG /MD /std:c++17 /I D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\src /I D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\thirdparty\lvgl /I D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\thirdparty\lvgl\src /I D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\thirdparty\sdl2-windows\include /I D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\thirdparty\freetype-mingw\include\freetype2 /DLV_CONF_INCLUDE_SIMPLE /DLV_LVGL_H_INCLUDE_SIMPLE /DLV_USE_FREETYPE=1 /DNOMINMAX /D_WIN32_WINNT=0x0A00 /c /nologo /W0 /Y-"

$src = "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\src\ui_main.cpp"
$out = "D:\temp_ui_main.obj"

Write-Host "Compiling ui_main.cpp..."
$proc = Start-Process -FilePath "cl.exe" -ArgumentList "$flags $src /Fo$out" -NoNewWindow -PassThru -RedirectStandardOutput "D:\cl_stdout.log" -RedirectStandardError "D:\cl_stderr.log"
$proc.WaitForExit()

Write-Host "Exit code: $($proc.ExitCode)"
if ($proc.ExitCode -ne 0) {
    Write-Host "=== STDOUT ==="
    Get-Content "D:\cl_stdout.log" | Select-Object -First 5
    Write-Host "=== STDERR (errors only) ==="
    Get-Content "D:\cl_stderr.log" | Select-String "error C|fatal error" | Select-Object -First 5
} else {
    Write-Host "Compilation succeeded"
}

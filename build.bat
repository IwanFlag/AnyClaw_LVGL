@echo off
call "C:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat"
cd /d C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL\build
cmake .. -G "NMake Makefiles"
if %errorlevel% neq 0 (
    echo CMake failed!
    pause
    exit /b %errorlevel%
)
nmake
if %errorlevel% neq 0 (
    echo NMake failed!
    pause
    exit /b %errorlevel%
)
echo Build successful!
pause

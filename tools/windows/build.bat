@echo off
REM ══════════════════════════════════════════════════════════════
REM  AnyClaw LVGL — Windows 原生编译脚本 (MSYS2/MinGW)
REM  前置条件: 安装 MSYS2 + MinGW-w64
REM    1. 下载 https://www.msys2.org
REM    2. pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2
REM  用法: 双击运行 或 在 MSYS2 MinGW64 终端中执行 tools\windows\build.bat
REM ══════════════════════════════════════════════════════════════

set PROJECT_DIR=%~dp0..\..
set BUILD_DIR=%PROJECT_DIR%\build-windows

echo ============================================================
echo   AnyClaw LVGL - Windows Native Build
echo   Project: %PROJECT_DIR%
echo   Build:   %BUILD_DIR%
echo ============================================================

REM 1. Clean
echo [1/3] Cleaning...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

REM 2. Configure
echo [2/3] Configuring...
cd /d "%BUILD_DIR%"
cmake "%PROJECT_DIR%" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo   X CMake configuration failed
    exit /b 1
)

REM 3. Build
echo [3/3] Building...
cmake --build . -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo   X Build failed
    exit /b 1
)

echo.
echo ============================================================
echo   + Build successful!
echo   Binary: %BUILD_DIR%\bin\AnyClaw_LVGL.exe
echo   DLL:    %BUILD_DIR%\bin\SDL2.dll
echo ============================================================

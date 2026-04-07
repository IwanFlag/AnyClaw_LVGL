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
set "CMAKE_BIN=cmake"
set "GENERATOR="
set "GEN_ARGS="

echo ============================================================
echo   AnyClaw LVGL - Windows Native Build
echo   Project: %PROJECT_DIR%
echo   Build:   %BUILD_DIR%
echo ============================================================

REM 0. Detect CMake (PATH first, then VS bundled)
where cmake >nul 2>nul
if errorlevel 1 (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "CMAKE_BIN=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    ) else (
        echo   X CMake not found. Please install CMake or Visual Studio CMake tools.
        exit /b 1
    )
)

REM Prefer VS generator on Windows native build; fallback to MinGW Makefiles.
where cl >nul 2>nul
if not errorlevel 1 (
    set "GENERATOR=Visual Studio 17 2022"
    set "GEN_ARGS=-A x64"
) else (
    set "GENERATOR=MinGW Makefiles"
    set "GEN_ARGS="
)

REM 1. Clean
echo [1/3] Cleaning...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

REM 2. Configure
echo [2/3] Configuring...
cd /d "%BUILD_DIR%"
"%CMAKE_BIN%" "%PROJECT_DIR%" -G "%GENERATOR%" %GEN_ARGS% -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo   X CMake configuration failed
    exit /b 1
)

REM 3. Build
echo [3/3] Building...
if /I "%GENERATOR%"=="Visual Studio 17 2022" (
    "%CMAKE_BIN%" --build . --config Release -j %NUMBER_OF_PROCESSORS%
) else (
    "%CMAKE_BIN%" --build . -j %NUMBER_OF_PROCESSORS%
)
if errorlevel 1 (
    echo   X Build failed
    exit /b 1
)

echo.
echo ============================================================
echo   + Build successful!
if /I "%GENERATOR%"=="Visual Studio 17 2022" (
    echo   Binary: %BUILD_DIR%\bin\Release\AnyClaw_LVGL.exe
    echo   DLL:    %BUILD_DIR%\bin\Release\SDL2.dll
) else (
    echo   Binary: %BUILD_DIR%\bin\AnyClaw_LVGL.exe
    echo   DLL:    %BUILD_DIR%\bin\SDL2.dll
)
echo ============================================================

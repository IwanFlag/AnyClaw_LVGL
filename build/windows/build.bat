@echo off
setlocal EnableDelayedExpansion
REM =============================================================
REM AnyClaw LVGL Windows build script
REM Prerequisite (optional MinGW): https://www.msys2.org
REM Usage: run this file from cmd/powershell
REM =============================================================

for %%I in ("%~dp0..\..") do set "PROJECT_DIR=%%~fI"
set "BUILD_DIR=%PROJECT_DIR%\out\windows"
set "CMAKE_BIN=cmake"
set "GENERATOR="
set "GEN_ARGS="
set "VSDEVCMD="
set "QUIET=1"
set "SHOW_HELP=0"
set "BUILD_LOG=%BUILD_DIR%\build.log"
set "CONFIG_LOG=%BUILD_DIR%\configure.log"

for %%A in (%*) do (
    if /I "%%~A"=="--quiet" set "QUIET=1"
    if /I "%%~A"=="--verbose" set "QUIET=0"
    if /I "%%~A"=="--help" set "SHOW_HELP=1"
    if /I "%%~A"=="-h" set "SHOW_HELP=1"
)

if "%SHOW_HELP%"=="1" (
    echo AnyClaw Windows build
    echo.
    echo Usage:
    echo   build\windows\build.bat [--quiet^|--verbose] [--help]
    echo.
    echo Options:
    echo   --quiet    Quiet mode ^(default^): only key progress, details to out\windows\*.log
    echo   --verbose  Show full configure/build output
    echo   --help    Show this help and exit
    exit /b 0
)

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

REM 0.5 Auto-load VS developer environment when cl.exe is not ready.
where cl >nul 2>nul
if errorlevel 1 (
    if exist "C:\VSBuildTools\Common7\Tools\VsDevCmd.bat" (
        set "VSDEVCMD=C:\VSBuildTools\Common7\Tools\VsDevCmd.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
        set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" (
        set "VSDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
    )

    if defined VSDEVCMD (
        echo [Auto] Loading VS dev environment...
        call "!VSDEVCMD!" -arch=x64 >nul 2>nul
    )
)

REM Prefer a matching VS generator when cl.exe is available; fallback to MinGW.
where cl >nul 2>nul
if not errorlevel 1 (
    if "%VisualStudioVersion%"=="18.0" (
        set "GENERATOR=Visual Studio 18 2026"
        set "GEN_ARGS=-A x64"
    ) else if "%VisualStudioVersion%"=="17.0" (
        set "GENERATOR=Visual Studio 17 2022"
        set "GEN_ARGS=-A x64"
    ) else (
        REM If version env is unavailable, use NMake as robust fallback.
        where nmake >nul 2>nul
        if not errorlevel 1 (
            set "GENERATOR=NMake Makefiles"
            set "GEN_ARGS="
        ) else (
            set "GENERATOR=Visual Studio 18 2026"
            set "GEN_ARGS=-A x64"
        )
    )
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
if "%QUIET%"=="1" (
    "%CMAKE_BIN%" -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" %GEN_ARGS% -DCMAKE_BUILD_TYPE=Release > "%CONFIG_LOG%" 2>&1
) else (
    "%CMAKE_BIN%" -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -G "%GENERATOR%" %GEN_ARGS% -DCMAKE_BUILD_TYPE=Release
)
if errorlevel 1 (
    REM Fallback: when version env is missing but only VS2022 is installed.
    if /I "%GENERATOR%"=="Visual Studio 18 2026" (
        echo   ! VS 2026 generator failed, retrying VS 2022 generator...
        if "%QUIET%"=="1" (
            "%CMAKE_BIN%" -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release >> "%CONFIG_LOG%" 2>&1
        ) else (
            "%CMAKE_BIN%" -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
        )
    )
)
if errorlevel 1 (
    echo   X CMake configuration failed
    if "%QUIET%"=="1" (
        echo   ! Error summary:
        powershell -NoProfile -Command "if (Test-Path '%CONFIG_LOG%') { Select-String -Path '%CONFIG_LOG%' -Pattern 'CMake Error|error:' | Select-Object -First 20 | ForEach-Object { $_.Line } }"
        echo   ! Last configure log lines:
        powershell -NoProfile -Command "if (Test-Path '%CONFIG_LOG%') { Get-Content -Path '%CONFIG_LOG%' -Tail 80 }"
    )
    exit /b 1
)

REM 3. Build
echo [3/3] Building...
echo %GENERATOR% | findstr /B /C:"Visual Studio" >nul
if not errorlevel 1 (
    if "%QUIET%"=="1" (
        "%CMAKE_BIN%" --build "%BUILD_DIR%" --config Release -j %NUMBER_OF_PROCESSORS% > "%BUILD_LOG%" 2>&1
    ) else (
        "%CMAKE_BIN%" --build "%BUILD_DIR%" --config Release -j %NUMBER_OF_PROCESSORS%
    )
) else (
    if "%QUIET%"=="1" (
        "%CMAKE_BIN%" --build "%BUILD_DIR%" -j %NUMBER_OF_PROCESSORS% > "%BUILD_LOG%" 2>&1
    ) else (
        "%CMAKE_BIN%" --build "%BUILD_DIR%" -j %NUMBER_OF_PROCESSORS%
    )
)
if errorlevel 1 (
    echo   X Build failed
    if "%QUIET%"=="1" (
        echo   ! Error summary:
        powershell -NoProfile -Command "if (Test-Path '%BUILD_LOG%') { Select-String -Path '%BUILD_LOG%' -Pattern 'fatal error| error [A-Z]+[0-9]+| error C[0-9]+| error LNK[0-9]+|: error ' | Select-Object -First 30 | ForEach-Object { $_.Line } }"
        echo   ! Last build log lines:
        powershell -NoProfile -Command "if (Test-Path '%BUILD_LOG%') { Get-Content -Path '%BUILD_LOG%' -Tail 80 }"
    )
    exit /b 1
)

echo.
echo ============================================================
exit /b 0
echo   + Build successful!
echo %GENERATOR% | findstr /B /C:"Visual Studio" >nul
if not errorlevel 1 (
    echo   Binary: %BUILD_DIR%\bin\Release\AnyClaw_LVGL.exe
    echo   DLL:    %BUILD_DIR%\bin\Release\SDL2.dll
) else (
    echo   Binary: %BUILD_DIR%\bin\AnyClaw_LVGL.exe
    echo   DLL:    %BUILD_DIR%\bin\SDL2.dll
)
echo ============================================================

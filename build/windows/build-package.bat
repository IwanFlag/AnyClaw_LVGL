@echo off
setlocal EnableDelayedExpansion
for %%I in ("%~dp0..\..") do set "PROJECT_DIR=%%~fI"
set "VSDEVCMD="
set "QUIET_ARG=--quiet"
set "SHOW_HELP=0"
set "ARTIFACTS_DIR=%PROJECT_DIR%\artifacts\windows-native"
set "EXE_PATH=%PROJECT_DIR%\out\windows\bin\Release\AnyClaw_LVGL.exe"
set "SDL_PATH=%PROJECT_DIR%\out\windows\bin\Release\SDL2.dll"
set "SDL_CACHE_PATH=%PROJECT_DIR%\thirdparty\sdl2-windows\lib\x64\SDL2.dll"

for %%A in (%*) do (
  if /I "%%~A"=="--quiet" set "QUIET_ARG=--quiet"
  if /I "%%~A"=="--verbose" set "QUIET_ARG="
  if /I "%%~A"=="--help" set "SHOW_HELP=1"
  if /I "%%~A"=="-h" set "SHOW_HELP=1"
)

if "%SHOW_HELP%"=="1" (
  echo AnyClaw Windows build + package
  echo.
  echo Usage:
  echo   build\windows\build-package.bat [--quiet^|--verbose] [--help]
  echo.
  echo Options:
  echo   --quiet    Quiet mode ^(default^): only key progress, save detailed logs
  echo   --verbose  Show full configure/build output
  echo   --help    Show this help and exit
  exit /b 0
)

REM Auto-load VS build environment for one-click usage.
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
    echo [Auto] Loading VS dev environment from: !VSDEVCMD!
    call "!VSDEVCMD!" -arch=x64
  )
)

call "%~dp0setup-env.bat" || exit /b 1
call "%~dp0build.bat" %QUIET_ARG% || exit /b 1

REM Normalize output paths for both VS multi-config and NMake single-config.
if not exist "%EXE_PATH%" (
  if exist "%PROJECT_DIR%\out\windows\bin\AnyClaw_LVGL.exe" (
    set "EXE_PATH=%PROJECT_DIR%\out\windows\bin\AnyClaw_LVGL.exe"
    set "SDL_PATH=%PROJECT_DIR%\out\windows\bin\SDL2.dll"
  )
)
if not exist "%SDL_PATH%" (
  if exist "%PROJECT_DIR%\out\windows\bin\SDL2.dll" (
    set "SDL_PATH=%PROJECT_DIR%\out\windows\bin\SDL2.dll"
  )
)

if not exist "%SDL_PATH%" (
  for %%I in ("%SDL_PATH%") do if not exist "%%~dpI" mkdir "%%~dpI" >nul 2>nul
  call "%~dp0fetch-runtime-deps.bat" || exit /b 1
)
if not exist "%SDL_PATH%" (
  echo [WARN] SDL2.dll missing in build output, trying local cache/thirdparty
  if exist "%SDL_CACHE_PATH%" copy /y "%SDL_CACHE_PATH%" "%SDL_PATH%" >nul
  if exist "%PROJECT_DIR%\thirdparty\SDL2\lib\x64\SDL2.dll" (
    copy /y "%PROJECT_DIR%\thirdparty\SDL2\lib\x64\SDL2.dll" "%SDL_PATH%" >nul
  )
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%\build\package_windows_full_runtime.ps1" ^
  -ProjectDir "%PROJECT_DIR%" -ExePath "%EXE_PATH%" -SdlPath "%SDL_PATH%" -OutRoot "%ARTIFACTS_DIR%"
if errorlevel 1 exit /b 1

echo [OK] Build + package done. Output root: %ARTIFACTS_DIR%
exit /b 0

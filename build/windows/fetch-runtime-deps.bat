@echo off
setlocal EnableDelayedExpansion
for %%I in ("%~dp0..\..") do set "PROJECT_DIR=%%~fI"
set "BUILD_ROOT=%PROJECT_DIR%\build\windows"
set "RUNTIME_ROOT=%BUILD_ROOT%\tools\runtime"
set "SDL_ROOT=%RUNTIME_ROOT%\SDL2"
set "SDL_DLL=%SDL_ROOT%\lib\x64\SDL2.dll"
set "OUT_DLL=%BUILD_ROOT%\out\bin\Release\SDL2.dll"
set "SDL_ZIP=%RUNTIME_ROOT%\SDL2-devel-2.30.11-VC.zip"
set "SDL_URL=https://github.com/libsdl-org/SDL/releases/download/release-2.30.11/SDL2-devel-2.30.11-VC.zip"

if exist "%SDL_DLL%" goto copy_dll

echo [AnyClaw] Preparing runtime deps in %RUNTIME_ROOT%
if not exist "%RUNTIME_ROOT%" mkdir "%RUNTIME_ROOT%"

where powershell >nul 2>nul
if errorlevel 1 (
  echo [ERR] PowerShell not found, cannot download SDL2 runtime.
  exit /b 1
)

if not exist "%SDL_ZIP%" (
  echo [DL] SDL2 runtime package...
  powershell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-WebRequest -Uri '%SDL_URL%' -OutFile '%SDL_ZIP%'"
  if errorlevel 1 (
    echo [ERR] Download SDL2 package failed.
    exit /b 1
  )
)

if not exist "%SDL_ROOT%" mkdir "%SDL_ROOT%"
echo [UNZIP] Extracting SDL2...
powershell -NoProfile -ExecutionPolicy Bypass -Command "Expand-Archive -Path '%SDL_ZIP%' -DestinationPath '%RUNTIME_ROOT%' -Force"
if errorlevel 1 (
  echo [ERR] Extract SDL2 package failed.
  exit /b 1
)

if not exist "%SDL_DLL%" (
  echo [ERR] SDL2.dll not found after extraction.
  exit /b 1
)

:copy_dll
for %%I in ("%OUT_DLL%") do if not exist "%%~dpI" mkdir "%%~dpI"
copy /y "%SDL_DLL%" "%OUT_DLL%" >nul
if errorlevel 1 (
  echo [ERR] Copy SDL2.dll to output failed.
  exit /b 1
)
echo [OK] SDL2 runtime ready: %OUT_DLL%
exit /b 0

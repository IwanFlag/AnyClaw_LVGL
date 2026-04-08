@echo off
setlocal
for %%I in ("%~dp0..\..") do set "PROJECT_DIR=%%~fI"
set "CACHE_DIR=%PROJECT_DIR%\tools\cache\sdl2-2.30.3"
set "SDL_DLL=%CACHE_DIR%\SDL2.dll"

if exist "%SDL_DLL%" (
  echo [OK] SDL2 runtime already cached: %SDL_DLL%
  exit /b 0
)

mkdir "%CACHE_DIR%" 2>nul
set "ZIP_PATH=%CACHE_DIR%\SDL2-2.30.3-win32-x64.zip"
echo [AnyClaw] Downloading SDL2 runtime once to cache...
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop'; Invoke-WebRequest -Uri 'https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-2.30.3-win32-x64.zip' -OutFile '%ZIP_PATH%'; Expand-Archive -Path '%ZIP_PATH%' -DestinationPath '%CACHE_DIR%' -Force; if (Test-Path '%CACHE_DIR%\SDL2.dll') { exit 0 } else { throw 'SDL2.dll missing after extract' }"
if errorlevel 1 exit /b 1
echo [OK] SDL2 runtime cached: %SDL_DLL%
exit /b 0

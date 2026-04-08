@echo off
setlocal
for %%I in ("%~dp0..\..") do set "PROJECT_DIR=%%~fI"
set "ARTIFACTS_DIR=%PROJECT_DIR%\artifacts\windows-native"
set "EXE_PATH=%PROJECT_DIR%\build-windows\bin\Release\AnyClaw_LVGL.exe"
set "SDL_PATH=%PROJECT_DIR%\build-windows\bin\Release\SDL2.dll"
set "SDL_CACHE_PATH=%PROJECT_DIR%\tools\cache\sdl2-2.30.3\SDL2.dll"

call "%~dp0setup-env.bat" || exit /b 1
call "%~dp0build.bat" || exit /b 1

if not exist "%SDL_PATH%" (
  call "%~dp0fetch-runtime-deps.bat" || exit /b 1
)
if not exist "%SDL_PATH%" (
  echo [WARN] SDL2.dll missing in build output, trying local cache/thirdparty
  if exist "%SDL_CACHE_PATH%" copy /y "%SDL_CACHE_PATH%" "%SDL_PATH%" >nul
  if exist "%PROJECT_DIR%\thirdparty\SDL2\lib\x64\SDL2.dll" (
    copy /y "%PROJECT_DIR%\thirdparty\SDL2\lib\x64\SDL2.dll" "%SDL_PATH%" >nul
  )
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%\tools\common\package_windows_full_runtime.ps1" ^
  -ProjectDir "%PROJECT_DIR%" -ExePath "%EXE_PATH%" -SdlPath "%SDL_PATH%" -OutRoot "%ARTIFACTS_DIR%"
if errorlevel 1 exit /b 1

echo [OK] Build + package done. Output root: %ARTIFACTS_DIR%
exit /b 0

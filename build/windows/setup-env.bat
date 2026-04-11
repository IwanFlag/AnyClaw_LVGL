@echo off
setlocal
for %%I in ("%~dp0..\..") do set "PROJECT_DIR=%%~fI"

echo [AnyClaw] Windows build environment check
echo Project: %PROJECT_DIR%

set "CMAKE_BIN=cmake"
where cmake >nul 2>nul
if errorlevel 1 (
  if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
    set "CMAKE_BIN=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    echo [OK] CMake found in VS bundled path
  ) else (
    echo [ERR] CMake not found
    exit /b 1
  )
) else (
  echo [OK] CMake found in PATH
)

where cl >nul 2>nul
if errorlevel 1 (
  echo [WARN] cl.exe not found in current shell. Please open Developer Command Prompt or start from VS terminal.
) else (
  echo [OK] MSVC cl.exe found
)

where git >nul 2>nul
if errorlevel 1 (
  echo [WARN] git not found in PATH
) else (
  echo [OK] git found
)

set "OUT_ENV=%~dp0.env-path.bat"
echo @echo off>"%OUT_ENV%"
echo set ANYCLAW_PROJECT_DIR=%PROJECT_DIR%>>"%OUT_ENV%"
echo set ANYCLAW_CMAKE_BIN=%CMAKE_BIN%>>"%OUT_ENV%"
echo set ANYCLAW_BUILD_ROOT=%PROJECT_DIR%\build\windows>>"%OUT_ENV%"
echo set ANYCLAW_BUILD_OUT=%PROJECT_DIR%\build\windows\out>>"%OUT_ENV%"
echo set ANYCLAW_BUILD_ARTIFACTS=%PROJECT_DIR%\build\windows\artifacts>>"%OUT_ENV%"
echo [OK] Generated %OUT_ENV%
echo Use: call build\windows\.env-path.bat
exit /b 0

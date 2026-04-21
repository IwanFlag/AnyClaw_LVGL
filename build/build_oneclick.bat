@echo off
setlocal

cd /d "%~dp0.."

echo [1/3] Build...
call build.bat
if errorlevel 1 (
	echo [ERROR] Build failed.
	exit /b 1
)

cd /d "%~dp0.."

if not exist "build\bin\AnyClaw_LVGL.exe" (
	echo [ERROR] build\bin\AnyClaw_LVGL.exe not found.
	exit /b 1
)

echo [2/3] Prepare package folder...
if exist "build\package" rmdir /s /q "build\package"
mkdir "build\package\AnyClaw_LVGL"
copy /y "build\bin\AnyClaw_LVGL.exe" "build\package\AnyClaw_LVGL\" >nul
if exist "build\bin\SDL2.dll" copy /y "build\bin\SDL2.dll" "build\package\AnyClaw_LVGL\" >nul
if exist "assets" xcopy /e /i /y "assets" "build\package\AnyClaw_LVGL\assets\" >nul

echo [3/3] Zip package...
powershell -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path 'build/package/AnyClaw_LVGL/*' -DestinationPath 'build/package/AnyClaw_LVGL_portable.zip' -Force"
if errorlevel 1 (
	echo [ERROR] Zip failed.
	exit /b 1
)

echo [OK] Done.
echo EXE: build\bin\AnyClaw_LVGL.exe
echo ZIP: build\package\AnyClaw_LVGL_portable.zip

endlocal

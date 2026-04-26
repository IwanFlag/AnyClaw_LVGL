@echo off
setlocal

cd /d "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\build"
call "C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 exit /b 1

if exist CMakeCache.txt (
	findstr /C:"CMAKE_GENERATOR:INTERNAL=Ninja" CMakeCache.txt >nul
	if errorlevel 1 (
		echo [INFO] Existing generator is not Ninja, reset CMake cache...
		if exist CMakeCache.txt del /f /q CMakeCache.txt
		if exist CMakeFiles rmdir /s /q CMakeFiles
	)
)

cmake -S .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

ninja
if errorlevel 1 exit /b 1

endlocal

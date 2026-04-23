@echo off
setlocal

cd /d "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\build"
call "C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 exit /b 1

cmake -S .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

ninja
if errorlevel 1 exit /b 1

endlocal

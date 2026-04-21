@echo off
pushd "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\build"
call "C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
nmake -f Makefile 2>&1
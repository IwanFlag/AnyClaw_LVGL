@echo off
call "C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=%PATH%;C:\Program Files\CMake\bin
"C:\Program Files\CMake\bin\cmake.exe" --build "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\build" 2>&1
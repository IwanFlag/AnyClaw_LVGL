@echo off
call "C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=%PATH%;C:\Program Files\CMake\bin
"C:\Program Files\CMake\bin\cmake.exe" -G "NMake Makefiles" -B "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\build" -DCMAKE_BUILD_TYPE=Release "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL"
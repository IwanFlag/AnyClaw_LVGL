@echo off
cd /d "D:\workspace\AnyClaw\AnyClaw_LVGL_Code\AnyClaw_LVGL\build"
call "C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
cmake -S .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
ninja

@echo off
call C:\VSBuildTools\VC\Auxiliary\Build\vcvars64.bat
cd /d C:\Users\thundersoft\.openclaw\workspace\AnyClaw_LVGL
if exist build rmdir /s /q build
mkdir build
cd build
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
if %ERRORLEVEL% neq 0 (
    echo CMAKE_CONFIGURE_FAILED
    exit /b 1
)
cmake --build . --config Release
echo BUILD_RESULT: %ERRORLEVEL%
echo BUILD_RESULT: %ERRORLEVEL%

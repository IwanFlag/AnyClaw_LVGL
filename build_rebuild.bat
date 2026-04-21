@echo off
call "C:\VSBuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
nmake -f Makefile
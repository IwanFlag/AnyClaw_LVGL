@echo off
setlocal
call "%~dp0build-package.bat" %*
exit /b %errorlevel%

@echo off
"%~dp0maze.exe"
set "result=%errorlevel%"
pause
exit /b %result%

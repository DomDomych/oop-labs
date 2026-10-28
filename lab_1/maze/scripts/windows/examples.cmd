@echo off
"%~dp0maze_example.exe"
set "result=%errorlevel%"
pause
exit /b %result%

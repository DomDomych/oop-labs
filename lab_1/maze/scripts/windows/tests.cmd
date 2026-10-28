@echo off
"%~dp0maze_tests.exe"
set "result=%errorlevel%"
pause
exit /b %result%

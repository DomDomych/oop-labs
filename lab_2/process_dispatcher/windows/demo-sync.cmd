@echo off
"%~dp0process_dispatcher.exe" "%~dp0configs\example.json" --mode sync
set "result=%errorlevel%"
echo Exit code: %result%
pause
exit /b %result%

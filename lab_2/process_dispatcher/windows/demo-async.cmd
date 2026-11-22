@echo off
"%~dp0process_dispatcher.exe" "%~dp0configs\example.json" --mode async
set "result=%errorlevel%"
echo Exit code: %result%
pause
exit /b %result%

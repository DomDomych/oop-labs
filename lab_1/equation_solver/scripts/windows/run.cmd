@echo off
"%~dp0eqsolver.exe"
set "result=%errorlevel%"
pause
exit /b %result%

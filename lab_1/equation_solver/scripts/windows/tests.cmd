@echo off
"%~dp0eqsolver_tests.exe"
set "result=%errorlevel%"
pause
exit /b %result%

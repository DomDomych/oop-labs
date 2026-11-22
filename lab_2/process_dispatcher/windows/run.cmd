@echo off
setlocal
if "%~1"=="" (
  "%~dp0process_dispatcher.exe" "%~dp0configs\example.json"
) else (
  "%~dp0process_dispatcher.exe" %*
)
exit /b %errorlevel%

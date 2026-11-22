@echo off
setlocal
"%~dp0dispatcher_unit_tests.exe" "%~dp0process_dispatcher.exe"
if errorlevel 1 exit /b %errorlevel%
python "%~dp0tests\integration.py" "%~dp0process_dispatcher.exe"
if errorlevel 1 exit /b %errorlevel%
python "%~dp0tools\check_reference.py" --program "%~dp0process_dispatcher.exe" --report "%~dp0output\reference-check.json"
exit /b %errorlevel%

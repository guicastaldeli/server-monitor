@echo off
setlocal

set ROOT_DIR=%~dp0
set BUILD_DIR=%ROOT_DIR%\.build

cd /d "%BUILD_DIR%" || (
    echo ERROR: Build folder not found.
    pause
    exit /b 1
)

if not exist server_monitor.exe (
    echo ERROR: server_monitor.exe not found. Run build.bat first.
    pause
    exit /b 1
)

echo Running...
server_monitor.exe %*
echo.
echo Exited with code %errorlevel%.
pause
endlocal
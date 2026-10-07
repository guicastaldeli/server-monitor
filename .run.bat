@echo off
setlocal

set ROOT_DIR=C:\Users\casta\OneDrive\Desktop\vscode\currency-monitor
set BUILD_DIR=%ROOT_DIR%\.build

cd /d "%BUILD_DIR%" || (
    echo ERROR: Build folder not found.
    pause
    exit /b 1
)

if not exist hello.exe (
    echo ERROR: hello.exe not found. Run build.bat first.
    pause
    exit /b 1
)

echo Running...
hello.exe
echo.
echo Exited with code %errorlevel%.
pause
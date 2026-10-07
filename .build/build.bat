@echo off
setlocal EnableDelayedExpansion

set ROOT_DIR=%~dp0..
set BUILD_DIR=%ROOT_DIR%\.build
set OUT_EXE=%BUILD_DIR%\server_monitor.exe

cd /d "%ROOT_DIR%" || (
    echo ERROR: Could not cd into %ROOT_DIR%
    pause
    exit /b 1
)

where g++ >nul 2>nul
if errorlevel 1 (
    echo ERROR: g++ not found on PATH.
    echo Add your MinGW bin folder to PATH and try again.
    pause
    exit /b 1
)

echo Building server-monitor...
echo =====================================================

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo.
echo Cleaning previous builds...
del /q "%BUILD_DIR%\*.exe" 2>nul

echo.
echo Collecting .cpp files...
set "CPP_LIST="
for /r "%ROOT_DIR%" %%f in (*.cpp) do (
    echo %%f | findstr /i /c:"\.build\\" >nul
    if errorlevel 1 (
        echo   %%f
        set "CPP_LIST=!CPP_LIST! "%%f""
    )
)

if "!CPP_LIST!"=="" (
    echo ERROR: No .cpp files found.
    pause
    exit /b 1
)

echo.
echo Compiling...
g++ -std=c++17 -O2 -static-libgcc -static-libstdc++ ^
    -I"%ROOT_DIR%" ^
    -o "%OUT_EXE%" ^
    !CPP_LIST! ^
    -lws2_32

if %errorlevel% neq 0 (
    echo ERROR: Build failed.
    pause
    exit /b 1
)

echo.
if exist "%OUT_EXE%" (
    echo BUILD SUCCESSFUL!
    echo Created: %OUT_EXE%
) else (
    echo ERROR: Output not created.
    pause
    exit /b 1
)

pause
endlocal
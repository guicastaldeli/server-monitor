@echo off
setlocal EnableDelayedExpansion

echo Building currency-monitor...
echo =====================================================

set ROOT_DIR=C:\Users\casta\OneDrive\Desktop\vscode\currency-monitor
set BUILD_DIR=%ROOT_DIR%\.build
set OUT_EXE=%BUILD_DIR%\hello.exe

set STORM_INC=%ROOT_DIR%\.lib\StormTrack\StormTrackHeaders
set STORM_LIB=%ROOT_DIR%\.lib\StormTrack\Release\StormTrack.lib
set DATA_LIB=%ROOT_DIR%\.lib\currency-lib\data.lib
set DATA_DLL=%ROOT_DIR%\.lib\currency-lib\data.dll
set DATA_HDR_DIR=%ROOT_DIR%\.lib\currency-lib

cd /d "%ROOT_DIR%" || (
    echo ERROR: Could not cd into %ROOT_DIR%
    pause
    exit /b 1
)

if /i "%CD%"=="%BUILD_DIR%" (
    echo ERROR!
    echo Run it from: %ROOT_DIR%
    pause
    exit /b 1
)

echo.
set VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build

if exist "%VS_PATH%\vcvars64.bat" (
    call "%VS_PATH%\vcvars64.bat"
    echo Building...
) else (
    echo ERROR: Visual Studio not found. Please install Visual Studio Build Tools.
    pause
    exit /b 1
)

echo.
echo Cleaning previous builds...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

del /q "%BUILD_DIR%\*.obj" 2>nul
del /q "%BUILD_DIR%\*.exe" 2>nul
del /q "%BUILD_DIR%\*.pdb" 2>nul
del /q "%BUILD_DIR%\*.ilk" 2>nul
del /q "%BUILD_DIR%\*.exp" 2>nul
del /q "%BUILD_DIR%\*.lib" 2>nul

echo.
echo Collecting .cpp files recursively...
set "CPP_LIST="
for /r "%ROOT_DIR%" %%f in (*.cpp) do (
    echo %%f | findstr /i /c:"\.build\\" >nul
    if errorlevel 1 (
        echo %%f | findstr /i /c:"\.lib\\StormTrack\\" >nul
        if errorlevel 1 (
            echo   %%f
            set "CPP_LIST=!CPP_LIST! "%%f""
        )
    )
)

if "!CPP_LIST!"=="" (
    echo ERROR: No .cpp files found in %ROOT_DIR%
    pause
    exit /b 1
)

echo.
echo Compiling with CL.EXE...
cl /nologo /c /O2 /EHsc /std:c++17 /MD ^
    /DUNICODE /D_UNICODE ^
    /Fo"%BUILD_DIR%\\" ^
    /I"%ROOT_DIR%" ^
    /I"%DATA_HDR_DIR%" ^
    /I"%STORM_INC%" ^
    !CPP_LIST!

if %errorlevel% neq 0 (
    echo ERROR: Compilation failed
    pause
    exit /b 1
)

echo.
echo Linking EXE with link.exe...
set "OBJ_LIST="
for %%f in ("%BUILD_DIR%\*.obj") do (
    set "OBJ_LIST=!OBJ_LIST! "%%f""
)

link /nologo ^
    /OUT:"%OUT_EXE%" ^
    /SUBSYSTEM:CONSOLE ^
    /ENTRY:wWinMainCRTStartup ^
    !OBJ_LIST! ^
    "%DATA_LIB%" ^
    "%STORM_LIB%" ^
    user32.lib ^
    shell32.lib ^
    gdi32.lib ^
    gdiplus.lib ^
    winmm.lib ^
    ole32.lib

if %errorlevel% neq 0 (
    echo ERROR: Linking EXE failed
    pause
    exit /b 1
)

echo.
echo Copying runtime DLL...
copy /Y "%DATA_DLL%" "%BUILD_DIR%\" >nul

echo.
echo Final verification...
if exist "%OUT_EXE%" (
    echo.
    echo BUILD SUCCESSFUL!
    echo Created: %OUT_EXE%
) else (
    echo ERROR: hello.exe not created!
    pause
    exit /b 1
)

echo.
pause
endlocal
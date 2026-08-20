@echo off
setlocal enabledelayedexpansion

echo === Building Zydis if needed ===

REM --- Detect platform (arg > env var > default x64) ---
set "PLAT=%~1"
if "%PLAT%"=="" set "PLAT=%Platform%"
if "%PLAT%"=="" set "PLAT=x64"

REM --- Zydis.vcxproj pins toolset v143 itself; set to 1 to override ---
set "ZY_FORCE_TOOLSET=1"

set "ZY_TOOLSET=%~2"
set "ZY_TOOLS_VER=%~3"

set "TOOLSET_ARGS="
if "%ZY_FORCE_TOOLSET%"=="1" (
    if defined ZY_TOOLSET set "TOOLSET_ARGS=!TOOLSET_ARGS! /p:PlatformToolset=!ZY_TOOLSET!"
    if defined ZY_TOOLS_VER set "TOOLSET_ARGS=!TOOLSET_ARGS! /p:VCToolsVersion=!ZY_TOOLS_VER!"
)

REM --- Locate Zydis project file ---
set "ZY_PROJECT=%~dp0external\zydis\msvc\zydis\Zydis.vcxproj"
if not exist %ZY_PROJECT% (
    echo ERROR: Zydis project file not found at %ZY_PROJECT%
    exit /b 1
)

REM --- Get current submodule commit hash ---
pushd "%~dp0external\zydis" >nul
for /f %%H in ('git rev-parse HEAD') do set "ZY_HASH=%%H"
if not defined ZY_HASH (
    echo ERROR: Could not read Zydis git hash. Is the submodule initialized?
    popd >nul
    exit /b 1
)
popd >nul

set "ZY_STAMP=%ZY_HASH%-%ZY_TOOLSET%-%ZY_TOOLS_VER%-f%ZY_FORCE_TOOLSET%"

REM --- Configure paths ---
if /i "%PLAT%"=="x64" (
    set "ZY_LIB=%~dp0external\zydis\msvc\bin\ReleaseX64\Zydis.lib"
    set "HASH_FILE=%~dp0external\zydis\msvc\bin\ReleaseX64\.zydis_build_hash"
    set "MSBUILD_ARGS=/p:Configuration="Release MT" /p:Platform=x64 /m /nologo /verbosity:minimal"
) else if /i "%PLAT%"=="Win32" (
    set "ZY_LIB=%~dp0external\zydis\msvc\bin\ReleaseX86\Zydis.lib"
    set "HASH_FILE=%~dp0external\zydis\msvc\bin\ReleaseX86\.zydis_build_hash"
    set "MSBUILD_ARGS=/p:Configuration="Release MT" /p:Platform=Win32 /m /nologo /verbosity:minimal"
) else (
    echo ERROR: Unsupported platform %PLAT%
    endlocal
    exit /b 1
)

REM --- Decide if rebuild is needed ---
set "NEED_BUILD=0"
if not exist "%ZY_LIB%" set "NEED_BUILD=1"
if not exist "%HASH_FILE%" set "NEED_BUILD=1"

if exist "%HASH_FILE%" (
    set "OLD_STAMP="
    set /p OLD_STAMP=<"%HASH_FILE%"
    if not "!OLD_STAMP!"=="%ZY_STAMP%" (
        echo [Zydis] Stamp changed:
        echo [Zydis]   cached: !OLD_STAMP!
        echo [Zydis]   wanted: %ZY_STAMP%
        set "NEED_BUILD=1"
    )
)

if "%NEED_BUILD%"=="1" (
    echo [Zydis] Clearing old output...
    rmdir /s /q "%~dp0external\zydis\msvc\bin\Release%PLAT%" 2>nul
    mkdir "%~dp0external\zydis\msvc\bin\Release%PLAT%"
)

REM --- Build if needed ---
if "%NEED_BUILD%"=="1" (
    echo [Zydis] Building Release MT %PLAT%
    msbuild %ZY_PROJECT% %MSBUILD_ARGS%!TOOLSET_ARGS!
    if errorlevel 1 (
        echo ERROR: Zydis Release MT %PLAT% build failed
        exit /b 1
    )
    >"%HASH_FILE%" echo %ZY_STAMP%
) else (
    echo [Zydis] Release MT %PLAT% up to date, skipping
)

echo === Zydis build check complete ===
endlocal
exit /b 0

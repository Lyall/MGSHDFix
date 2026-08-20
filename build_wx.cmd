@echo off
setlocal enabledelayedexpansion

echo === Building wxWidgets if needed ===

set "WX_TOOLSET=%~1"
set "WX_TOOLS_VER=%~2"

set "MSBUILD_PROPS="
if defined WX_TOOLSET set "MSBUILD_PROPS=!MSBUILD_PROPS! /p:PlatformToolset=!WX_TOOLSET!"
if defined WX_TOOLS_VER set "MSBUILD_PROPS=!MSBUILD_PROPS! /p:VCToolsVersion=!WX_TOOLS_VER!"

REM --- Locate wx build folder ---
set "WX_BUILD_DIR=%~dp0external\wxWidgets\build\msw"
if not exist "%WX_BUILD_DIR%" (
    echo ERROR: wxWidgets build folder not found at %WX_BUILD_DIR%
    exit /b 1
)

REM --- Resolve latest wxWidgets .sln dynamically (highest vcXX) ---
set "WX_SLN="
set "WX_MAX_VER=0"

for %%F in ("%WX_BUILD_DIR%\wx_vc*.sln") do (
    REM Example filename (no ext): wx_vc17
    set "FILE=%%~nF"
    set "WX_VER=!FILE:wx_vc=!"

    REM Ensure WX_VER is numeric only
    set "BAD="
    for /f "delims=0123456789" %%X in ("!WX_VER!") do set "BAD=%%X"
    if defined BAD (
        set "WX_VER="
    )

    if defined WX_VER (
        if !WX_VER! GTR !WX_MAX_VER! (
            set "WX_MAX_VER=!WX_VER!"
            set "WX_SLN=%%F"
        )
    )
)

if not defined WX_SLN (
    echo ERROR: Could not find wxWidgets Visual Studio solution in %WX_BUILD_DIR%
    exit /b 1
)

echo [wxWidgets] Using solution: %WX_SLN%
if defined WX_TOOLSET (
    echo [wxWidgets] PlatformToolset: !WX_TOOLSET!  VCToolsVersion: !WX_TOOLS_VER!
) else (
    echo [wxWidgets] WARNING: no toolset supplied, wx_config.props will choose one.
    echo [wxWidgets] WARNING: this can mismatch the linking project's toolset.
)

REM --- Get current submodule commit hash ---
pushd "%~dp0external\wxWidgets" >nul
for /f %%H in ('git rev-parse HEAD') do set "WX_HASH=%%H"
if not defined WX_HASH (
    echo ERROR: Could not read wxWidgets git hash. Is the submodule initialized?
    popd >nul
    exit /b 1
)
popd >nul

REM --- Stamp: source hash + the toolset that produced the libs ---
set "WX_STAMP=%WX_HASH%-%WX_TOOLSET%-%WX_TOOLS_VER%"

REM --- Paths for artifacts and stored stamp ---
set "WX_LIB_DIR=%WX_BUILD_DIR%\..\..\lib\vc_x64_lib"
set "HASH_FILE=%WX_LIB_DIR%\.wx_build_hash"

REM --- Flags ---
set "NEED_RELEASE_BUILD=0"
set "NEED_DEBUG_BUILD=0"
set "DID_REBUILD=0"

REM --- Check Release needs rebuild ---
if not exist "%WX_LIB_DIR%\mswu\wx\setup.h" set "NEED_RELEASE_BUILD=1"
if not exist "%HASH_FILE%" set "NEED_RELEASE_BUILD=1"

if exist "%HASH_FILE%" (
    set "OLD_STAMP="
    set /p OLD_STAMP=<"%HASH_FILE%"
    if not "!OLD_STAMP!"=="%WX_STAMP%" (
        echo [wxWidgets] Stamp changed:
        echo [wxWidgets]   cached: !OLD_STAMP!
        echo [wxWidgets]   wanted: %WX_STAMP%
        set "NEED_RELEASE_BUILD=1"
    )
)

REM --- If Release rebuilds, force Debug too ---
if "%NEED_RELEASE_BUILD%"=="1" set "NEED_DEBUG_BUILD=1"

REM --- Otherwise check Debug independently (only if not CI) ---
if /i not "%CI%"=="true" (
    if not exist "%WX_LIB_DIR%\mswud\wx\setup.h" set "NEED_DEBUG_BUILD=1"
    if not exist "%HASH_FILE%" set "NEED_DEBUG_BUILD=1"
    if exist "%HASH_FILE%" (
        set "OLD_STAMP="
        set /p OLD_STAMP=<"%HASH_FILE%"
        if not "!OLD_STAMP!"=="%WX_STAMP%" set "NEED_DEBUG_BUILD=1"
    )
)

REM --- If stamp is outdated, wipe the lib folder ---
if "%NEED_RELEASE_BUILD%"=="1" (
    echo [wxWidgets] Clearing old libraries...
    rmdir /s /q "%WX_LIB_DIR%" 2>nul
    mkdir "%WX_LIB_DIR%" 2>nul
)

REM --- Build Release ---
if "%NEED_RELEASE_BUILD%"=="1" (
    echo [wxWidgets] Building Release...
    msbuild "%WX_SLN%" /p:Configuration=Release /p:Platform=x64 !MSBUILD_PROPS! /m /t:Rebuild
    if errorlevel 1 (
        echo ERROR: Release build failed.
        exit /b 1
    )
    set "DID_REBUILD=1"
) else (
    echo [wxWidgets] Release build up to date, skipping.
)

REM --- Build Debug (only if not CI) ---
if /i not "%CI%"=="true" (
    if "%NEED_DEBUG_BUILD%"=="1" (
        echo [wxWidgets] Building Debug...
        msbuild "%WX_SLN%" /p:Configuration=Debug /p:Platform=x64 !MSBUILD_PROPS! /m /t:Rebuild
        if errorlevel 1 (
            echo ERROR: Debug build failed.
            exit /b 1
        )
        set "DID_REBUILD=1"
    ) else (
        echo [wxWidgets] Debug build up to date, skipping.
    )
) else (
    echo [wxWidgets] Skipping Debug build in CI.
)

REM --- Update stamp only after all builds succeed ---
if "%DID_REBUILD%"=="1" (
    >"%HASH_FILE%" echo %WX_STAMP%
)

echo === wxWidgets build check complete ===
endlocal

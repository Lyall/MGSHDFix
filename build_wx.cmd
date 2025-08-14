@echo off
setlocal enabledelayedexpansion

echo === Building full wxWidgets solution if needed ===

REM --- Locate wx build folder ---
set "WX_BUILD_DIR=%~dp0external\wxWidgets\build\msw"
if not exist "%WX_BUILD_DIR%" (
    echo ERROR: wxWidgets build folder not found at %WX_BUILD_DIR%
    exit /b 1
)

REM --- Find solution file dynamically ---
set "WX_SLN="
for %%F in ("%WX_BUILD_DIR%\wx_vc*.sln") do (
    set "WX_SLN=%%F"
    goto :found_sln
)

:found_sln
if "%WX_SLN%"=="" (
    echo ERROR: Could not find wxWidgets Visual Studio solution in %WX_BUILD_DIR%
    exit /b 1
)

REM --- Release build ---
if not exist "%WX_BUILD_DIR%\..\..\lib\vc_x64_lib\mswu\wx\setup.h" (
    echo [wxWidgets] Building full Release...
    msbuild "%WX_SLN%" /p:Configuration=Release /p:Platform=x64 /m
) else (
    echo [wxWidgets] Release build already present, skipping.
)

REM --- Debug build ---
if not exist "%WX_BUILD_DIR%\..\..\lib\vc_x64_lib\mswud\wx\setup.h" (
    echo [wxWidgets] Building full Debug...
    msbuild "%WX_SLN%" /p:Configuration=Debug /p:Platform=x64 /m
) else (
    echo [wxWidgets] Debug build already present, skipping.
)

echo === wxWidgets full build check complete ===
endlocal

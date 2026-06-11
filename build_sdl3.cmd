@echo off
setlocal enabledelayedexpansion

echo === Building SDL3 if needed ===

REM --- Locate SDL folder ---
set "SDL_DIR=%~dp0external\SDL"
if not exist "%SDL_DIR%\CMakeLists.txt" (
    echo ERROR: SDL folder not found at %SDL_DIR%
    exit /b 1
)

REM --- Check CMake ---
where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: CMake was not found.
    echo Install a recent CMake version or add it to PATH.
    exit /b 1
)

REM --- Check MSVC environment ---
where cl >nul 2>nul
if errorlevel 1 (
    echo ERROR: cl.exe was not found.
    echo Run this from a Visual Studio Developer Command Prompt.
    echo For example: x64 Native Tools Command Prompt for VS 2022
    exit /b 1
)

REM --- Locate vswhere ---
set "VSWHERE="

where vswhere >nul 2>nul
if not errorlevel 1 (
    set "VSWHERE=vswhere.exe"
) else (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
        set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    )
)

REM --- Check Visual Studio / CMake compatibility ---
set "HAS_SUPPORTED_VS=0"
set "HAS_VS2026=0"
set "HAS_VS2022=0"
set "CMAKE_HAS_VS2026=0"
set "CMAKE_HAS_VS2022=0"

cmake --help | findstr /C:"Visual Studio 18 2026" >nul 2>nul
if not errorlevel 1 set "CMAKE_HAS_VS2026=1"

cmake --help | findstr /C:"Visual Studio 17 2022" >nul 2>nul
if not errorlevel 1 set "CMAKE_HAS_VS2022=1"

if defined VSWHERE (
    "%VSWHERE%" -version "[18.0,19.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath >nul 2>nul
    if not errorlevel 1 set "HAS_VS2026=1"

    "%VSWHERE%" -version "[17.0,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath >nul 2>nul
    if not errorlevel 1 set "HAS_VS2022=1"

    if "!HAS_VS2026!"=="1" if "!CMAKE_HAS_VS2026!"=="1" set "HAS_SUPPORTED_VS=1"
    if "!HAS_VS2022!"=="1" if "!CMAKE_HAS_VS2022!"=="1" set "HAS_SUPPORTED_VS=1"

    if "!HAS_SUPPORTED_VS!"=="0" (
        echo WARNING: Installed CMake version may not support any installed Visual Studio version.
        echo.
        echo Detected:
        if "!HAS_VS2026!"=="1" (
            echo - Visual Studio 2026 is installed.
        ) else (
            echo - Visual Studio 2026 is not installed.
        )
        if "!HAS_VS2022!"=="1" (
            echo - Visual Studio 2022 is installed.
        ) else (
            echo - Visual Studio 2022 is not installed.
        )
        if "!CMAKE_HAS_VS2026!"=="1" (
            echo - Installed CMake version supports the Visual Studio 18 2026 generator.
        ) else (
            echo - Installed CMake version does not support the Visual Studio 18 2026 generator.
        )
        if "!CMAKE_HAS_VS2022!"=="1" (
            echo - Installed CMake version supports the Visual Studio 17 2022 generator.
        ) else (
            echo - Installed CMake version does not support the Visual Studio 17 2022 generator.
        )
        echo.
        echo The installed CMake version must support the generator for one of your installed Visual Studio versions.
        echo Continuing anyway. CMake configure will report the final error if this is actually invalid.
        echo.
    )
) else (
    echo WARNING: vswhere.exe was not found. Skipping Visual Studio/CMake compatibility precheck.
    echo CMake configure will report the final error if Visual Studio cannot be found.
)

REM --- Get current submodule commit hash ---
pushd "%SDL_DIR%" >nul
for /f %%H in ('git rev-parse HEAD') do set "SDL_HASH=%%H"
if not defined SDL_HASH (
    echo ERROR: Could not read SDL git hash. Is the submodule initialized?
    popd >nul
    exit /b 1
)
popd >nul

REM --- Paths for artifacts and stored hash ---
set "SDL_BUILD_DIR=%SDL_DIR%\build\msvc-x64-static"
set "SDL_LIB_DIR=%SDL_BUILD_DIR%\Release"
set "HASH_FILE=%SDL_BUILD_DIR%\.sdl_build_hash"

REM --- Flags ---
set "NEED_RELEASE_BUILD=0"
set "DID_REBUILD=0"

REM --- Check Release needs rebuild ---
if not exist "%SDL_LIB_DIR%\SDL3-static.lib" set "NEED_RELEASE_BUILD=1"
if not exist "%HASH_FILE%" set "NEED_RELEASE_BUILD=1"

if exist "%HASH_FILE%" (
    set "OLD_HASH="
    set /p OLD_HASH=<"%HASH_FILE%"
    if not "!OLD_HASH!"=="%SDL_HASH%" set "NEED_RELEASE_BUILD=1"
)

REM --- If hash is outdated, wipe the build folder ---
if "%NEED_RELEASE_BUILD%"=="1" (
    echo [SDL3] Clearing old build files...
    rmdir /s /q "%SDL_BUILD_DIR%" 2>nul
    mkdir "%SDL_BUILD_DIR%" 2>nul
)

REM --- Configure and build Release ---
if "%NEED_RELEASE_BUILD%"=="1" (
    echo [SDL3] Configuring Release...

    cmake -S "%SDL_DIR%" -B "%SDL_BUILD_DIR%" ^
        -A x64 ^
        -T v143 ^
        -DSDL_SHARED=OFF ^
        -DSDL_STATIC=ON ^
        -DSDL_TESTS=OFF ^
        -DSDL_EXAMPLES=OFF

    if errorlevel 1 (
        echo ERROR: SDL3 configure failed.
        echo.
        echo Make sure you have:
        echo - Visual Studio 2026 or Visual Studio 2022
        echo - Desktop development with C++
        echo - MSVC v143 - VS 2022 C++ x64/x86 build tools
        echo - Windows 10/11 SDK
        echo - A recent CMake version that supports your installed Visual Studio version
        exit /b 1
    )

    echo [SDL3] Building Release...

    cmake --build "%SDL_BUILD_DIR%" --config Release --parallel

    if errorlevel 1 (
        echo ERROR: SDL3 Release build failed.
        echo Make sure MSVC v143 and the Windows SDK are installed.
        exit /b 1
    )

    set "DID_REBUILD=1"
) else (
    echo [SDL3] Release build up to date, skipping.
)

REM --- Update hash only after build succeeds ---
if "%DID_REBUILD%"=="1" (
    >"%HASH_FILE%" echo %SDL_HASH%
)

echo === SDL3 build check complete ===
endlocal
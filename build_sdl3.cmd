@echo off
setlocal enabledelayedexpansion

echo === Building SDL3 if needed ===

set "SDL_CACHE_VERSION=2-vs2022-v143-x64-static"
set "SDL_CMAKE_GENERATOR=Visual Studio 17 2022"
set "SDL_CMAKE_PLATFORM=x64"
set "SDL_CMAKE_TOOLSET=v143"

set "SDL_DIR=%~dp0external\SDL"
if not exist "%SDL_DIR%\CMakeLists.txt" (
    echo ERROR: SDL folder not found at %SDL_DIR%
    exit /b 1
)

set "SDL_BUILD_DIR=%SDL_DIR%\build\msvc-x64-static"
set "SDL_LIB_DIR=%SDL_BUILD_DIR%\Release"
set "HASH_FILE=%SDL_BUILD_DIR%\.sdl_build_hash"
set "CACHE_VERSION_FILE=%SDL_BUILD_DIR%\.sdl_cache_version"

where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: CMake was not found.
    echo Install CMake or add it to PATH.
    exit /b 1
)

cmake --help | findstr /C:"%SDL_CMAKE_GENERATOR%" >nul 2>nul
if errorlevel 1 (
    echo ERROR: Installed CMake does not support the "%SDL_CMAKE_GENERATOR%" generator.
    echo Install a newer CMake version.
    exit /b 1
)

set "VSWHERE="
where vswhere >nul 2>nul
if not errorlevel 1 (
    set "VSWHERE=vswhere.exe"
) else (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
        set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    )
)

if defined VSWHERE (
    "%VSWHERE%" -products * -requires Microsoft.VisualStudio.Component.VC.v143.x86.x64 -property installationPath >nul 2>nul
    if errorlevel 1 (
        echo ERROR: MSVC v143 build tools were not found.
        echo Install "MSVC v143 - VS 2022 C++ x64/x86 build tools" and a Windows SDK.
        exit /b 1
    )
) else (
    echo WARNING: vswhere.exe was not found. Skipping v143 precheck.
)

set "SDL_HASH="
pushd "%SDL_DIR%" >nul
for /f %%H in ('git rev-parse HEAD 2^>nul') do set "SDL_HASH=%%H"
popd >nul

if not defined SDL_HASH (
    echo ERROR: Could not read SDL git hash. Is the submodule initialized?
    exit /b 1
)

set "NEED_RELEASE_BUILD=0"
set "DID_REBUILD=0"
set "INVALIDATE_BUILD_DIR=0"

if not exist "%SDL_LIB_DIR%\SDL3-static.lib" set "NEED_RELEASE_BUILD=1"
if not exist "%HASH_FILE%" set "NEED_RELEASE_BUILD=1"
if not exist "%CACHE_VERSION_FILE%" set "NEED_RELEASE_BUILD=1"

if exist "%HASH_FILE%" (
    set "OLD_HASH="
    set /p OLD_HASH=<"%HASH_FILE%"
    if not "!OLD_HASH!"=="%SDL_HASH%" (
        echo [SDL3] SDL submodule hash changed.
        set "NEED_RELEASE_BUILD=1"
        set "INVALIDATE_BUILD_DIR=1"
    )
)

if exist "%CACHE_VERSION_FILE%" (
    set "OLD_CACHE_VERSION="
    set /p OLD_CACHE_VERSION=<"%CACHE_VERSION_FILE%"
    if not "!OLD_CACHE_VERSION!"=="%SDL_CACHE_VERSION%" (
        echo [SDL3] SDL build cache version changed.
        echo [SDL3] Old: !OLD_CACHE_VERSION!
        echo [SDL3] New: %SDL_CACHE_VERSION%
        set "NEED_RELEASE_BUILD=1"
        set "INVALIDATE_BUILD_DIR=1"
    )
)

if exist "%SDL_BUILD_DIR%\CMakeCache.txt" (
    findstr /C:"CMAKE_GENERATOR:INTERNAL=%SDL_CMAKE_GENERATOR%" "%SDL_BUILD_DIR%\CMakeCache.txt" >nul 2>nul
    if errorlevel 1 (
        echo [SDL3] Existing CMake cache uses a different generator.
        set "NEED_RELEASE_BUILD=1"
        set "INVALIDATE_BUILD_DIR=1"
    )

    findstr /C:"CMAKE_GENERATOR_PLATFORM:INTERNAL=%SDL_CMAKE_PLATFORM%" "%SDL_BUILD_DIR%\CMakeCache.txt" >nul 2>nul
    if errorlevel 1 (
        echo [SDL3] Existing CMake cache uses a different platform.
        set "NEED_RELEASE_BUILD=1"
        set "INVALIDATE_BUILD_DIR=1"
    )

    findstr /C:"CMAKE_GENERATOR_TOOLSET:INTERNAL=%SDL_CMAKE_TOOLSET%" "%SDL_BUILD_DIR%\CMakeCache.txt" >nul 2>nul
    if errorlevel 1 (
        echo [SDL3] Existing CMake cache uses a different toolset.
        set "NEED_RELEASE_BUILD=1"
        set "INVALIDATE_BUILD_DIR=1"
    )
)

if not exist "%SDL_BUILD_DIR%\CMakeCache.txt" (
    if exist "%SDL_BUILD_DIR%" (
        echo [SDL3] Build folder exists but CMakeCache.txt is missing.
        set "NEED_RELEASE_BUILD=1"
        set "INVALIDATE_BUILD_DIR=1"
    )
)

if "%INVALIDATE_BUILD_DIR%"=="1" (
    echo [SDL3] Clearing stale build files...
    rmdir /s /q "%SDL_BUILD_DIR%" 2>nul
)

if "%NEED_RELEASE_BUILD%"=="1" (
    if not exist "%SDL_BUILD_DIR%" mkdir "%SDL_BUILD_DIR%" >nul 2>nul

    echo [SDL3] Configuring Release...

    cmake -S "%SDL_DIR%" -B "%SDL_BUILD_DIR%" ^
        -G "%SDL_CMAKE_GENERATOR%" ^
        -A %SDL_CMAKE_PLATFORM% ^
        -T %SDL_CMAKE_TOOLSET% ^
        -DSDL_SHARED=OFF ^
        -DSDL_STATIC=ON ^
        -DSDL_TESTS=OFF ^
        -DSDL_EXAMPLES=OFF

    if errorlevel 1 (
        echo ERROR: SDL3 configure failed.
        echo.
        echo SDL3 is built using:
        echo - Generator: %SDL_CMAKE_GENERATOR%
        echo - Platform:  %SDL_CMAKE_PLATFORM%
        echo - Toolset:   %SDL_CMAKE_TOOLSET%
        echo.
        echo Make sure CMake supports "%SDL_CMAKE_GENERATOR%" and MSVC v143 is installed.
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

if "%DID_REBUILD%"=="1" (
    >"%HASH_FILE%" echo %SDL_HASH%
    >"%CACHE_VERSION_FILE%" echo %SDL_CACHE_VERSION%
)

echo === SDL3 build check complete ===
endlocal
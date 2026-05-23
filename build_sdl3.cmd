@echo off
setlocal enabledelayedexpansion

echo === Building SDL3 if needed ===

REM --- Locate SDL folder ---
set "SDL_DIR=%~dp0external\SDL"
if not exist "%SDL_DIR%\CMakeLists.txt" (
    echo ERROR: SDL folder not found at %SDL_DIR%
    exit /b 1
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
        -T v145 ^
        -DSDL_SHARED=OFF ^
        -DSDL_STATIC=ON ^
        -DSDL_TESTS=OFF ^
        -DSDL_EXAMPLES=OFF

    if errorlevel 1 (
        echo ERROR: SDL3 configure failed.
        exit /b 1
    )

    echo [SDL3] Building Release...
    cmake --build "%SDL_BUILD_DIR%" --config Release --parallel

    if errorlevel 1 (
        echo ERROR: SDL3 Release build failed.
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
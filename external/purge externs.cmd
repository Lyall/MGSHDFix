@echo off
setlocal

cd /d "%~dp0"

git rev-parse --git-dir >nul 2>nul
if errorlevel 1 (
    echo ERROR: not a git repository.
    exit /b 1
)

echo === Deinitializing submodules ===
git submodule deinit -f --all
if errorlevel 1 exit /b 1

echo.
echo === Re-checking out submodules ===
git submodule update --init --recursive
if errorlevel 1 exit /b 1

echo.
echo === Done ===

endlocal

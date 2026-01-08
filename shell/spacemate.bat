@echo off
setlocal

REM Get the directory where the script is located
set "SCRIPT_DIR=%~dp0"
set "WSL_PATH=/mnt/c%SCRIPT_DIR:~2:-1:\=/%"

if "%1"=="" (
    echo Running SpaceMate GUI...
    wsl -e bash -ic "cd '%WSL_PATH%' && ./run_spacemate.sh gui"
) else if "%1"=="gui" (
    echo Running SpaceMate GUI...
    wsl -e bash -ic "cd '%WSL_PATH%' && ./run_spacemate.sh gui"
) else if "%1"=="cli" (
    echo Running SpaceMate CLI...
    wsl -e bash -ic "cd '%WSL_PATH%' && ./run_spacemate.sh cli"
) else (
    echo Usage: spacemate [gui^|cli]
    echo.
    echo Examples:
    echo   spacemate        # Run GUI version
    echo   spacemate gui    # Run GUI version
    echo   spacemate cli    # Run CLI version
)
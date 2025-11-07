@echo off
if "%1"=="" (
    echo Running SpaceMate GUI...
    wsl -e bash -ic "/mnt/c/Users/jmana/OneDrive/Desktop/Spacemate/run_spacemate.sh gui"
) else if "%1"=="gui" (
    echo Running SpaceMate GUI...
    wsl -e bash -ic "/mnt/c/Users/jmana/OneDrive/Desktop/Spacemate/run_spacemate.sh gui"
) else if "%1"=="cli" (
    echo Running SpaceMate CLI...
    wsl -e bash -ic "/mnt/c/Users/jmana/OneDrive/Desktop/Spacemate/run_spacemate.sh cli"
) else (
    echo Usage: run_spacemate [gui^|cli]
    echo.
    echo Examples:
    echo   run_spacemate        # Run GUI version
    echo   run_spacemate gui    # Run GUI version
    echo   run_spacemate cli    # Run CLI version
)
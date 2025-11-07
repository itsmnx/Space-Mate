@echo off
echo Building Spacemate...

REM Create build directory if it doesn't exist
if not exist "build" mkdir build
cd build

REM Run CMake
echo Running CMake...
cmake ..

REM Build the project
echo Building project...
cmake --build .

REM Check if build was successful
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful!
    echo.
    echo To run the GUI version:
    echo    build\SpacemateGUI.exe
    echo.
    echo To run the CLI version:
    echo    build\spacemate_cli.exe
) else (
    echo.
    echo Build failed with error code %ERRORLEVEL%
)

cd ..
pause
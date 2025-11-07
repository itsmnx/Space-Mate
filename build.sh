#!/bin/bash

# Exit on error
set -e

echo "Building Spacemate..."

# Remove existing build directory if it exists
if [ -d "build" ]; then
    rm -rf build
fi

# Create build directory
mkdir build
cd build

# Run CMake
echo "Running CMake..."
cmake ..

# Build the project
echo "Building project..."
make -j4

# Check if build was successful
if [ $? -eq 0 ]; then
    echo
    echo "Build successful!"
    echo
    echo "To run the GUI version:"
    echo "    ./SpacemateGUI"
    echo
    echo "To run the CLI version:"
    echo "    ./spacemate_cli"
else
    echo
    echo "Build failed!"
fi

cd ..
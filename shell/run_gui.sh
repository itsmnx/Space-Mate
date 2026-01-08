#!/bin/bash

# Get the WSL IP address for display
export DISPLAY=$(grep -m 1 nameserver /etc/resolv.conf | awk '{print $2}'):0.0

# Test X server connection
if ! xset q &>/dev/null; then
    echo "Error: X server not running or not accessible"
    echo "Please make sure VcXsrv is running and 'Disable access control' is checked"
    exit 1
fi

# Set Qt platform
export QT_QPA_PLATFORM=xcb

# Go to build directory
cd "$(dirname "$0")/build"

# Run the GUI
./SpacemateGUI
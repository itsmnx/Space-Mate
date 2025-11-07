#!/bin/bash

# Function to display usage
show_usage() {
    echo "SpaceMate Runner"
    echo "Usage:"
    echo "  ./run_spacemate.sh [gui|cli]"
    echo
    echo "Examples:"
    echo "  ./run_spacemate.sh gui    # Run the GUI version"
    echo "  ./run_spacemate.sh cli    # Run the CLI version"
}

# Set up X11 display for GUI
setup_display() {
    export DISPLAY=:0
    export QT_QPA_PLATFORM=xcb
}

# Default to GUI if no argument is provided
MODE=${1:-gui}

case $MODE in
    gui)
        echo "Starting SpaceMate GUI..."
        setup_display
        cd "$(dirname "$0")/build" && ./SpacemateGUI
        ;;
    cli)
        echo "Starting SpaceMate CLI..."
        cd "$(dirname "$0")/build" && ./spacemate_cli
        ;;
    *)
        show_usage
        exit 1
        ;;
esac
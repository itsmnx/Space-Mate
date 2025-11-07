#!/bin/bash

# Get IP address
export DISPLAY=$(ip route | grep default | awk '{print $3; exit}'):0.0

# Enable X11 forwarding for all clients
xhost +local:

# Print status
echo "DISPLAY set to: $DISPLAY"
echo "X11 forwarding enabled"
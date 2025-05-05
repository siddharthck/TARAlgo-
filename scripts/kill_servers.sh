#!/bin/bash

# Script to kill all server processes
echo "Killing all server processes..."

# Use pkill to terminate all processes with the name 'server'
pkill -9 server

# Check if the processes were successfully killed
if [ $? -eq 0 ]; then
    echo "All server processes have been terminated."
else
    echo "No server processes found or failed to terminate."
fi
#!/usr/bin/env bash

# Print "Hello, World!" to the console every 5 seconds as a background 
# Making sure this continues after the script completes execution (nohup, and disown process)
while true; do
    echo "Hello, World!"
    sleep 5
done &

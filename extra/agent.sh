#!/usr/bin/env bash

# Print "Hello, World!" to the console every 5 seconds as a background 
# Making sure this continues after the script completes execution (nohup, and disown process)
echo "Starting Dyno Metrics Agent... v0.02"

# Start the agent (agent)
~/bin/agent &

#!/usr/bin/env bash

# Print "Hello, World!" to the console every 5 seconds as a background 
# Making sure this continues after the script completes execution (nohup, and disown process)
echo "Hello, World!"

# Dump all the environment variables to the console:
echo "Environment variables:"
printenv

while true; do
    echo "source=web.1 dyno=heroku.2808254.d97d0ea7-cf3d-411b-b453-d2943a50b456 sample#load_avg_1m=2.46 sample#load_avg_5m=1.06 sample#load_avg_15m=0.99"
    sleep 20
done &

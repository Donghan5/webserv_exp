#!/bin/bash
PID=$1
while true; do
    echo "$(date) - Open FDs: $(ls -l /proc/$PID/fd | wc -l) - Children: $(ps --ppid $PID | wc -l)"
    sleep 1
done

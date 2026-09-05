#!/bin/bash

URL="YOUR_GAS_WEB_APP_URL"

while true
do

    curl -4 -L -s \
      --connect-timeout 10 \
      --max-time 30 \
      "$URL" \
      -o /tmp/timer_new.json

    if [ $? -eq 0 ] && [ -s /tmp/timer_new.json ]; then

    cat /tmp/timer_new.json > /tmp/timer.json
    rm -f /tmp/timer_new.json

fi

    sleep 1

done

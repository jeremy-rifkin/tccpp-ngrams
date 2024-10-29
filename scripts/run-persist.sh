#!/bin/bash

while true
do
    NODE_ENV=production unbuffer npx tsx server/app.ts 2>&1 | tee -a log.txt
    sleep 10
done

#!/bin/bash

if [ -f log.txt ]; then
    echo "Previous log file detected"
    [ ! -d log_archive ] && mkdir log_archive
    t=$(date -r log.txt "+%m.%d.%Y::%H:%M:%S")
    gzip log.txt
    mv -v log.txt.gz log_archive/log.$t.txt.gz
fi

npm i && npm run build

screen -dmLS _Tccppngrams ./scripts/run-persist.sh

echo "tccpp-ngrams started in screen"

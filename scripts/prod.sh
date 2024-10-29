#!/bin/bash

rsync -r package.json package-lock.json tsconfig.json server shared ui scripts ngrams.duckdb server@d0:dist/tccpp-ngrams --checksum
ssh server@d0 "cd dist/tccpp-ngrams && screen -XS _Tccppngrams quit; ./scripts/start.sh"

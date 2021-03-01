#!/bin/bash
if [ ! -f "xynq.build" ]; then
    echo "Should run from build/ directory"
    exit -1
fi

docker build -t xynq .

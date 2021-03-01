#!/bin/bash
if [ ! -f "xynq.build" ]; then
    echo "Should run from build/ directory"
    exit -1
fi

docker run --rm -it -p 9920:9920 -w /xynq/bin/Linux-x86_64 -v ${PWD}/..:/xynq xynq /bin/bash

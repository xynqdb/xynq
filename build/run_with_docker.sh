#!/bin/bash
if [ ! -f "xynq.build" ]; then
    echo "Should run from build/ directory"
    exit -1
fi

docker run --rm -it -v ${PWD}/..:/xynq xynq /xynq/bin/Linux-x86_64/$1

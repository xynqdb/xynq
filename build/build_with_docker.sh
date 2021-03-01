#!/bin/bash
if [ ! -f "xynq.build" ]; then
    echo "Should run from build/ directory"
    exit -1
fi

docker run --rm -it -v ${PWD}/..:/xynq -e "CMAKE_BUILD_TYPE=$1" xynq /xynq/build/docker_build.sh

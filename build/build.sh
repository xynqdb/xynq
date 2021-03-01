#!/bin/bash

if [ ! -f "xynq.build" ]; then
    echo "Should run from build/ directory"
    exit -1
fi

BINTEMP=../bin_temp/$1

mkdir -p ${BINTEMP}
pushd ${BINTEMP}
cmake ../../build/ -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
make
popd

#!/bin/bash

# exit when any command fails
set -e

cd $(dirname $0)
pwd

cmake \
        -B build_native \
        -DCMAKE_BUILD_TYPE=Debug \
        -S test
cmake --build build_native --verbose

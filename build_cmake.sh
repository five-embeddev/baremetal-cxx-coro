#!/bin/bash

# exit when any command fails
set -e

cd $(dirname $0)
pwd

cmake \
        -DCMAKE_TOOLCHAIN_FILE=`pwd`/cmake/riscv.cmake \
        -B build \
        -S src 
cmake --build build --verbose


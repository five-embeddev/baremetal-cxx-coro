#!/bin/bash

ALL_SRC=`find src/ include/ -name '*.hpp' -or -name '*.cpp' -type f `
echo "FORMAT: ${ALL_SRC}"
clang-format -i ${ALL_SRC}

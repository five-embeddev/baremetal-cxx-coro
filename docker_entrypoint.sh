#!/bin/bash
# USAGE: ./docker_entrypoint.sh <native or target> <path to git volume> <path to project source with CMakelist.txt>

time=$(date)
echo "::set-output name=start_time::$time"

build_type=$1
# Remove $1 from args.
shift

if [ -d $1 ] ; then
    echo "Changing to working directory: $1"
    cd $1
else
    echo "Not changing directory: $1"
fi

# Remove $1 from args.
shift 

# Default path
if [ "$#" == "0" ]; then
	echo "$USAGE"
	exit 1
fi

BUILD_BASE=build_docker
CMAKE_DIR=cmake

rc=0

PROJECT_SRC=baremetal-cxx-coro-dev

for target in $build_type ; do
    BUILD_DIR=${BUILD_BASE}_$target
    
    echo "Using: BUILD_DIR=${BUILD_DIR}, PROJECT_SRC=${PROJECT_SRC}"
    
    rm -rf  ${BUILD_DIR}
    mkdir -p ${BUILD_DIR}

    if [ "$target" == "target" ] ; then
        EXTRA_ARGS="-DCMAKE_TOOLCHAIN_FILE=${CMAKE_DIR}/riscv.cmake"
    else
        EXTRA_ARGS="-DCMAKE_BUILD_TYPE=Debug"
    fi
    
    cmake \
       -DCMAKE_MAKE_PROGRAM=make \
       ${EXTRA_ARGS} \
       -G "Unix Makefiles" \
       -B ${BUILD_DIR} \
       -S .
    cmake --build ${BUILD_DIR}
    
    if [ "$?" != "0" ] ; then
       echo "CMAKE: ${PROJECT_SRC}; Build failed: $?"
       rc=$[$rc + 1]
    else
       echo "CMAKE: ${PROJECT_SRC}; Build success"
    fi

done

echo "RUN TESTS IN ${BUILD_BASE}_native"
${BUILD_BASE}_native/test/unit_tests
pushd ${BUILD_BASE}_native
ctest \
    --output-junit results.xml
popd

if [ "$?" != "0" ] ; then
   echo "CMAKE: ${PROJECT_SRC}; Test failed: $?"
   rc=$[$rc + 1]
else
   echo "CMAKE: ${PROJECT_SRC}; Test success"
fi

    
time=$(date)
echo "::set-output name=end_time::$time"

exit $rc

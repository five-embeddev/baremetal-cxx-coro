cmake_minimum_required(VERSION 3.15)

project(baremetal_cxx_coro CXX)
include(CTest)
enable_testing()

set(MY_CMAKE_FLAGS "" CACHE STRING "Additional CMake command line flags")

option(ENABLE_FUZZING "Enable Fuzzing Builds" OFF)
option(ENABLE_ASAN "Enable Address Sanitize Builds" OFF)

if(ENABLE_ASAN)
  add_compile_options(-fsanitize=address -fsanitize=leak )
  add_link_options(-fsanitize=address)
endif()

#   -fno-exceptions
#   -fno-use-cxa-atexit

add_compile_options(
  -std=c++20
  -Os
  -g
  -Wall
  -ffunction-sections
  -fno-rtti
  -fno-nonansi-builtins
  -fno-threadsafe-statics
  -nostartfiles
  -fcoroutines
  -Wextra
  -Wpedantic
  -fno-diagnostics-color
  -Wno-unused-function
  -Werror
  )

include_directories(${PROJECT_SOURCE_DIR}/include)

set ( STACK_SIZE 0xf00 )

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "${CMAKE_HOST_SYSTEM_PROCESSOR}")
  add_compile_options(-DHOST_EMULATION -D__riscv_xlen=32)
else()
  add_compile_options(-march=${CMAKE_SYSTEM_PROCESSOR})

  SET(LINKER_SCRIPT "${PROJECT_SOURCE_DIR}/src/linker.lds")
  SET(MY_CMAKE_FLAGS "-DCMAKE_TOOLCHAIN_FILE=${PROJECT_SOURCE_DIR}/cmake/riscv.cmake")

  # Linker control
  add_link_options(
    -nostartfiles
    -Xlinker
    --defsym=__stack_size=${STACK_SIZE}
    -T ${LINKER_SCRIPT}
    -Wl,-Map=${TARGET}.map)

endif()

set(CMAKE_CXX_STANDARD 20)
SET(DCMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
add_subdirectory(test)

if(ENABLE_FUZZING)
  message("Building Fuzz Tests, using fuzzing sanitizer https://www.llvm.org/docs/LibFuzzer.html")
  add_subdirectory(test/fuzz)
endif()

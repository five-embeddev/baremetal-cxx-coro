# Baremetal C++20 Co-Routines for RISC-V

Example of using baremetal C++ Co-routines with RISC-V.

The code will initialize the C++ run-time environment and jump to main
and create a set of periodic co-routines. The co-routines will be
timed using the machine mode timer and scheduled using the event loop
interrupted by the machine mode timer.

## Files

Source Code

- src/startup.cpp - Entry/Startup/Runtime
- src/main.cpp - Main Program, Interrupt Handler, Co-routine examples
- include/coro
- include/coro/nop_task.hpp - C++20 co-routine task
- include/coro/scheduler.hpp - C++20 co-routine scheduler
- include/coro/awaitable_timer.hpp - C++20 awaitable timer concept
- include/riscv
- include/riscv/timer.hpp - RISC-V Timer Driver
- include/riscv/riscv-csr.hpp / 
- include/riscv/riscv-interrupts.hpp - RISC-V Hardware Support
- include/native

Platform IO:

- platformio.ini - IDE Configuration
- post_build.py - Example post build to create disassembly file

CMake Build Environment:

- Makefile
- CMakeLists.txt
- src/CMakeLists.txt
- tests/CMakeLists.txt
- cmake/riscv.cmake

GitHub/Docker CI

- Dockerfile
- docker_entrypoint.sh
- action.yml
- .github/workflows/main.yml

Source Code Check/Formatting

- make cppcheck - Run clang-tidy and cppcheck 
- make clang_tidy_native - Run clang-tidy in native mode
- make clang_tidy_target - Run clang-tidy in target mode
- make format - Run clang-format


## Building

Platform IO or CMake is used to build the project locally.

Docker is used to build the project on github.

### PlatformIO

Platform IO will download the required toolchain to build and QEMU to run the target locally. This is using q quite an old GCC (8.3.0).

~~~
platformio run --target build
~~~

### CMake

To build with CMake a RISC-V cross compiler must be installed. It will look for one of:

- riscv-none-embed-g++
- riscv-none-elf-g++
- riscv32-unknown-elf-g++

The tools can be found at: https://xpack.github.io/dev-tools/riscv-none-elf-gcc/

~~~
	cmake \
			${CMAKE_OPTIONS_${@}} \
            -DCMAKE_TOOLCHAIN_FILE=cmake/riscv.cmake \
	        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		    -B build_target \
	        -S .
	cmake --build build_target --verbose
~~~

#### Testing

The `Makefile` has targets to build with CMake and run tests with CTest.

~~~
make native
nake native_test
~~~


### Docker

The included dockerfile installs the [xpack RISC-V GCC
toolchain](https://xpack.github.io/riscv-none-embed-gcc/) and uses
CMake to compile the project. This is using a recent GCC (12.2).

To build locally using the docker file use these commands:

~~~
docker build --tag=cxx_coro_riscv:latest .
docker run \
       -it \
       -v `pwd`:/work \
       cxx_coro_riscv:latest \
       /work \
       ./

~~~

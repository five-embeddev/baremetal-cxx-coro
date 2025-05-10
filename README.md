# Baremetal C++20 Co-Routines for RISC-V

Example of using baremetal C++ Co-routines with RISC-V.

Details in the following articles
- [Exploring C++20 coroutines for embedded](https://philmulholland.medium.com/c-20-coroutines-re-entrant-scheduled-tasks-no-os-required-061c20efafad) ([alt link](https://five-embeddev.com/articles/2024/11/24/part-2-cpp20-coroutines-short/))
- [Building a header-only C++20 coroutine runtime](https://philmulholland.medium.com/c-20-coroutines-a-header-only-runtime-for-re-entrant-tasks-87cb8e2c0ee1) ([alt link](https://five-embeddev.com/articles/2024/11/24/part-1-cpp20-coroutines-runtime/))

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

#  Pre Commit

https://pre-commit.com/ is configured to some basic linting.

# Using the library

### A simple coroutines task

A simple task `periodic` is defined in [example_simple.cpp](https://github.com/five-embeddev/baremetal-cxx-coro/blob/main/src/example_simple.cpp#L30).
It takes `scheduler`, `period` and `resume_count` as arguments and asynchronously waits `period` microseconds for 10 iterations, updating the `resume_count` value each iteration. 

The `scheduler` passed as an argument is not strictly necessary for C++ coroutines, but is used to make the ownership of the context of each task explicit. (It could be possible to use a global scheduler, such as when implementing via OS threads.)

The task returns `nop_task`. This is a special structure that is linked to the coroutines implementation. In this case a "nop task" refers to a task that does not return a value via `co_return`. 

```

template<typename SCHEDULER>
nop_task periodic(
    SCHEDULER& scheduler,
    std::chrono::microseconds period,
    volatile uint32_t& resume_count) {
    driver::timer<> mtimer;
    for (auto i = 0; i < 10; i++) {
        co_await scheduled_delay{ scheduler, period };
        *timestamp_resume[resume_count] = mtimer.get_time<driver::timer<>::timer_ticks>().count();
        resume_count = i + 1;
    }
    co_return; // Not strictly needed
}
```


The function has the following behavior:

- Take `period` as a parameter in microseconds.
- Keep track of the number of iteraton via the `resume_count` counter.
- Iterate 10 times.
- For each iteration, 
   - wait `period` using the `co_await` keyword.
   - then increment `resume_count`.
- Use `co_return` to exit the coroutines.

The following sequence diagram shows an abstract coroutine execution where an abstracted OS exists to handle the scheduling of process execution. ([PlantUML source](https://github.com/five-embeddev/baremetal-cxx-coro/blob/main/docs/diagrams/tast_sequence_abstract.puml))

![Task Sequence](/docs/diagrams/tast_sequence_abstract.svg)


### Calling the simple coroutine task

The `example_simple()` function in [example_simple.cpp](https://github.com/five-embeddev/baremetal-cxx-coro/blob/main/src/example_simple.cpp#L41) calls the `periodic` function once, with `100ms` as the `period` value. 

The `scheduler_delay<mtimer_clock>` is a scheduler class that will manage the software timer to wake each coroutine at the appropriate time, using our RISC-V machine mode timer driver `mtimer`.

```
    driver::timer<> mtimer;
    // Class to manage timer coroutines
    scheduler_delay<mtimer_clock> scheduler;
    // Run two concurrent loops. The first loop will run concurrently to the second loop.
    auto t0 = periodic(scheduler, 100ms, resume_simple);
```

### Resuming the coroutine tasks

For this example the scheduler is an object instantiated in the `example_simple()` function. It needs to be called explicitly to calculate when each coroutine needs to be woken and resumed. This is a convention of the runtime for this example, and not a required convention for C++ coroutines.

The tasks are resumed in the [WFI busy loop of `example_Simple()`](https://github.com/five-embeddev/baremetal-cxx-coro/blob/main/src/example_simple.cpp#L82) when `scheduler.update()` is called. However, as the scheduler is just a C++ class, this can be called from other locations, such as a timer interrupt handler.

```
    do {
        // Get a delay to the next coroutines wake up
        schedule_by_delay<mtimer_clock> now;
        auto [pending, next_wake] = scheduler.update(now);
        if (pending) {
            // Next wakeup
            mtimer.set_time_cmp(next_wake->delay());
            // Timer interrupt enable
            riscv::csrs.mstatus.mie.clr();
            riscv::csrs.mie.mti.set();
            // WFI Should be called while interrupts are disabled 
            // to ensure interrupt enable and WFI is atomic.            
            core.wfi();
        ]
    } while(true)
```

For example as the IRQ handler in this example is a lambda function, we could also capture the scheduler and run the timer coroutine in the IRQ handler.

```
    static const auto handler = [&](void) {
        ...
        schedule_by_delay<mtimer_clock> now;
        auto [pending, next_wake] = scheduler.update(now);
    };
```

# Design

### Summary of the runtime files 

The runtime for this example is a set of include files in [`include/coro`](https://github.com/five-embeddev/baremetal-cxx-coro/tree/main/include/coro). These files are used:

- [`nop_task.hpp`](https://github.com/five-embeddev/baremetal-cxx-coro/tree/main/include/coro/nop_task.hpp) : Task structure including `promise_type` to conform the C++ coroutines task concept.
- [`scheduler.hpp`](https://github.com/five-embeddev/baremetal-cxx-coro/tree/main/include/coro/scheduler.hpp) : Generic scheduler class that can manage a set of `std::coroutine_handle` to determine when they should resume and implement the resumption.
- [`awaitable_timer.hpp`](https://github.com/five-embeddev/baremetal-cxx-coro/tree/main/include/coro/awaitable_timer.hpp) : An "awaitable" class that can be used with `co_await` to schedule a coroutines to wake up after a given `std::chono` delay.
- [`static_list.hpp`](https://github.com/five-embeddev/baremetal-cxx-coro/tree/main/include/coro/static_list.hpp): An alternative to `std::list` that uses custom memory allocation from a static region to avoid heap usage. 
- [`awaitable_priority.hpp`](https://github.com/five-embeddev/baremetal-cxx-coro/tree/main/include/coro/awaitable_priority.hpp): An alternative "awaitable" class for tasks to be scheduled to wake according to priority.

**NOTE:** All classes here are designed to not use the heap for allocation. They will allocate all memory from statically declared buffers.

<!--more-->

### The coroutine task concept

The `nop_task` class in [`nop_task.hpp`](https://github.com/five-embeddev/baremetal-cxx-coro/tree/main/include/coro/nop_task.hpp) file implements the coroutine task concept.

 A coroutine task includes a promise concept with no return values. The important structures in this file are``struct nop_task`` / ``struct nop_task::promise_type``. This is implemented as described in [CPP Reference](https://en.cppreference.com/w/cpp/language/coroutines).

This task structure will be allocated each time a coroutine is called. To avoid heap allocation static memory allocation is used (to be described below). When using a memory constrained platform it is important to understand that the number of coroutines that can be called is restricted by the memory allocated for `nop_task::task_heap_`.

The relationships between the task classes is shown in the following class diagram:

![Task](/docs/diagrams/nop_task.svg)

### The awaitable concept

The classes in [`awaitable_timer.hpp`](https://github.com/five-embeddev/baremetal-cxx-coro/tree/main/include/coro/awaitable_timer.hpp) and [`awaitable_priority.hpp`](https://github.com/five-embeddev/baremetal-cxx-coro/tree/main/include/coro/awaitable_priority.hpp) represent asynchronous events that pause the coroutine task until an event occurs.

These classes are designed to be returned from a `co_await`, this ensures a task can be scheduled to be resumed on a later event.

The `awaitable_timer` class implements the awaitable concept described in [CPP Reference](https://en.cppreference.com/w/cpp/language/coroutines), and also the `co_await` operator that is overloaded to take the `scheduler_delay` struct and return `awaitable_timer`. An additional concept of the `scheduler` class is being used to manage the coroutine handle and wake up conditions that are used to implement coroutine task pause.

The relationships between the awaitable classes is shown in the following class diagram:

![Awaitable](/docs/diagrams/awaitable.svg)

### The scheduler class

The classes in [`scheduler.hpp`](https://github.com/five-embeddev/baremetal-cxx-coro/tree/main/include/coro/scheduler.hpp) are designed to do the work of managing the coroutines that are paused. It is a template class, parameterized according to the type of event that should be scheduled, and the maximum number of concurrent active tasks. 

The scheduler does the work that would be done by an RTOS or General Purpose OS. It manages a task list of waiting tasks with wake conditions and resumes them on the wake event. 

 The awaitable classes, introduced above, will insert paused tasks via `insert()`. The active execution context must call `resume()` to resume the paused tasks. Each entry in the task list is a `schedule_entry` structure. The classe are templates specialized by the wake up condition.

This scheduler class is not a concept required by C++ coroutines, but in this example it is needed as there is no operating system scheduler.

The relationships between scheduler classes is shown in the following class diagram:

![Software Timer](/docs/diagrams/scheduler.svg)


### Using the awaitable and scheduler classes to create a software timer

The awaitable class and scheduler are combined to implement the software timer feature. The following diagram shows how the classes relate.

![Software Timer](/docs/diagrams/software_timer.svg)

### Walk through of the detailed sequence of suspend and resume  

Now the concrete classes have been defined, the sequence to suspend and resume a coroutine class can be show.

It is shown below in 3 stages in relation to the simple timer example.

#### 1. Setup a coroutine, and suspend on the first wait. 

![Task Sequence](/docs/diagrams/task_sequence.svg)

#### 2. Resume and suspend, iterate over several time delays.

![Task Sequence](/docs/diagrams/task_sequence_001.svg)

#### 3. Complete iterating and exit coroutines.

![Task Sequence](/docs/diagrams/task_sequence_002.svg)


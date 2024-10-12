/*
   Baremetal main program with co-routines drivem by timer interrupt.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#include <cstdint>
#include <chrono>

#include "embeddev_riscv.hpp"
#include "embeddev_coro.hpp"

#include "example_simple.hpp"
#include "example_irq.hpp"
#include "example_timer.hpp"

static volatile bool TEST_SIMPLE = true;
static volatile bool TEST_IRQ = false;
static volatile bool TEST_TIMERS = false;

int main(int argc, const char** argv) {
#if defined(HOST_EMULATION)
    driver::timer<> mtimer;
    riscv_cpu_t core{ argc, argv, riscv::csrs, mtimer };
#else
    (void)argc;
    (void)argv;
    riscv_cpu_t core;
#endif

    if (TEST_SIMPLE) {
        example_simple(core);
    }
    if (TEST_IRQ) {
        example_irq(core);
    }
    if (TEST_TIMERS) {
        example_timer(core);
    }
    return 0;
}

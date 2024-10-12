/*
   Unit tests priority scheduled co-routines.

   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/


#include <cstdint>
#include <coroutine>
#include <chrono>

#ifdef HOST_EMULATION
#include <iostream>
#include <thread>
#endif

#include "unity.h"

#include "coro/scheduler.hpp"
#include "coro/nop_task.hpp"
#include "coro/awaitable_priority.hpp"

template<typename SCHEDULER>
nop_task resuming_on_priority(
    SCHEDULER& scheduler,
    const unsigned int run_count,
    volatile unsigned int& resume_count) {
    for (unsigned int i = 0; i < run_count; i++) {
        co_await scheduled_priority{ scheduler, (int)i };
        resume_count = i + 1;
    }
}

void test_single_prio_coroutine(void) {
    // Inttialize coroutine
    // Class to manage timer co-routines
    scheduler_priority coro_scheduler;
    unsigned int resume_count{ 0 };
    constexpr unsigned int iterations = 10;

    auto task = resuming_on_priority(coro_scheduler, iterations, resume_count);

    do {
        (void)coro_scheduler.resume(schedule_by_priority{ 0 });
    } while (!task.done());
    TEST_ASSERT_EQUAL_UINT(resume_count, iterations);
}

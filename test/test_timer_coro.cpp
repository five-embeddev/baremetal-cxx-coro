/*
   Unit tests timer scheduled co-routines.

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
#include "coro/awaitable_timer.hpp"

#ifdef HOST_EMULATION
#include "host/timer.hpp"
#else
#include "riscv/riscv-csr.hpp"
#include "riscv/riscv-isa.hpp"
#include "riscv/timer.hpp"
#include "riscv/scheduler-timer-mtimer.hpp"
#endif

#ifdef HOST_EMULATION
using test_clock = std::chrono::steady_clock;
void sleep_for(test_clock::duration delay) {
    std::this_thread::sleep_for(delay);
}
#else
using test_clock = mtimer_clock;
void sleep_for(test_clock::duration delay) {
    static riscv::cpu core;
    static driver::timer<> mtimer;
    // Next wakeup
    mtimer.set_time_cmp(delay);
    // Timer interrupt enable
    riscv::csrs.mie.mti.set();
    core.wfi();
}
#endif


/**  A simple task to schedule
 * @tparam SCHEDULER    The type of scheduler that will manage this co-routine's execution.
 * @param scheduler     The actual of scheduler that will manage this co-routine's execution.
 * @param period        This co-routine will periodically wake up with this period.
 * @param resume_count  Count the number of times this co-routine wakes up. For introspection only.
 */
template<typename SCHEDULER>
nop_task resuming_on_delay(
    SCHEDULER& scheduler,
    std::chrono::microseconds period,
    const unsigned int run_count,
    volatile unsigned int& resume_count) {
    for (unsigned int i = 0; i < run_count; i++) {
        co_await scheduled_delay{ scheduler, period };
        resume_count = i + 1;
    }
}


void test_single_coroutine(void) {
    // Inttialize coroutine
    // Class to manage timer co-routines
    scheduler_delay<test_clock> coro_scheduler;
    unsigned int resume_count{ 0 };
    constexpr unsigned int iterations = 10;
    constexpr auto delay = 100ms;
    constexpr auto total_time = delay * iterations;
    auto task = resuming_on_delay(coro_scheduler, delay, iterations, resume_count);

    const auto start_time = test_clock::now();
    auto elapsed_time = start_time - test_clock::now();
    do {
        schedule_by_delay<test_clock> now;
        auto [pending, next_wake] = coro_scheduler.update(now);
        if (next_wake) {
            sleep_for(next_wake->delay());
        }
        elapsed_time = start_time - test_clock::now();
    } while (!task.done());
    TEST_ASSERT_EQUAL_UINT(resume_count, iterations);
}

void test_interleaving_coroutines(void) {

    // Inttialize coroutine
    // Class to manage timer co-routines
    scheduler_delay<test_clock> coro_scheduler;
    unsigned int resume_count1{ 0 };
    unsigned int resume_count2{ 0 };
    unsigned int resume_count3{ 0 };
    constexpr unsigned int iterations1 = 11;
    constexpr unsigned int iterations2 = 12;
    constexpr unsigned int iterations3 = 13;
    constexpr auto delay1 = 121ms;
    constexpr auto delay2 = 133ms;
    constexpr auto delay3 = 145ms;
    constexpr auto total_time = delay3 * iterations3;


    auto task1 = resuming_on_delay(coro_scheduler, delay1, iterations1, resume_count1);
    auto task2 = resuming_on_delay(coro_scheduler, delay2, iterations2, resume_count2);
    auto task3 = resuming_on_delay(coro_scheduler, delay3, iterations3, resume_count3);

    const auto start_time = test_clock::now();
    auto elapsed_time = start_time - test_clock::now();
    do {
        schedule_by_delay<test_clock> now;
        auto [pending, next_wake] = coro_scheduler.update(now);
        if (next_wake) {
            sleep_for(next_wake->delay());
        }
        elapsed_time = start_time - test_clock::now();
    } while (!(task1.done() && task2.done() && task3.done()));

    TEST_ASSERT_EQUAL_UINT(resume_count1, iterations1);
    TEST_ASSERT_EQUAL_UINT(resume_count2, iterations2);
    TEST_ASSERT_EQUAL_UINT(resume_count3, iterations3);
}

template<typename SCHEDULER>
nop_task nested_level2(
    SCHEDULER& scheduler,
    unsigned int& collect_flags) {
    collect_flags |= 0x10;
    co_await scheduled_delay{ scheduler, 124ms };
    collect_flags |= 0x20;
    co_await scheduled_delay{ scheduler, 33ms };
    collect_flags |= 0x40;
}

template<typename SCHEDULER>
nop_task nested_level1(
    SCHEDULER& scheduler,
    unsigned int& collect_flags) {

    collect_flags |= 0x1;
    co_await scheduled_delay{ scheduler, 24ms };
    collect_flags |= 0x2;
    auto task2 = nested_level2(scheduler, collect_flags);
    collect_flags |= 0x4;
    (void)task2;// TODO - what happens here.
}


void test_nested_coroutines(void) {

    scheduler_delay<test_clock> coro_scheduler;
    unsigned int cover_flags{ 0 };

    constexpr auto total_time = 24ms + 124ms + 33ms + 1ms;// 1ms margin

    auto task1 = nested_level1(coro_scheduler, cover_flags);

    const auto start_time = test_clock::now();
    auto elapsed_time = start_time - test_clock::now();
    do {
        schedule_by_delay<test_clock> now;
        auto [pending, next_wake] = coro_scheduler.update(now);
        if (next_wake) {
            sleep_for(next_wake->delay());
        }
        elapsed_time = start_time - test_clock::now();
    } while (!(task1.done()) || !coro_scheduler.empty());

    TEST_ASSERT_EQUAL_HEX(0x77, cover_flags);
}

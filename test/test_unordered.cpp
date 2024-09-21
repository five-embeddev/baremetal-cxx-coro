/*
   Unit tests unordered scheduled co-routines.

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
#include "coro/awaitable_unordered.hpp"

template<class SCHEDULER>
nop_task single_coro_loop(SCHEDULER& a,
                          const unsigned int run_count,
                          volatile unsigned int& resume_count) {

    for (unsigned int i = 0; i < run_count; i++) {
        co_await a;
        resume_count = i + 1;
    }
}

template<class SCHEDULER>
nop_task double_coro_loop(SCHEDULER& a,
                          SCHEDULER& b,
                          const unsigned int run_count,
                          volatile unsigned int& resume_count_a,
                          volatile unsigned int& resume_count_b) {

    for (unsigned int i = 0; i < run_count; i++) {
        co_await a;
        resume_count_a = i + 1;
        co_await b;
        resume_count_b = i + 1;
    }
}


void test_single_unordered_coroutine(void) {
    // Inttialize coroutine
    // Class to manage timer co-routines
    scheduler_unordered a;
    unsigned int resume_count{ 0 };
    constexpr unsigned int iterations = 10;

    auto task = single_coro_loop(a, iterations, resume_count);

    do {
        a.resume();
    } while (!task.done());
    TEST_ASSERT_EQUAL_UINT(resume_count, iterations);
}

void test_double_unordered_coroutine(void) {
    // Inttialize coroutine
    // Class to manage timer co-routines
    scheduler_unordered a;
    scheduler_unordered b;
    unsigned int resume_count_a{ 0 };
    unsigned int resume_count_b{ 0 };
    constexpr unsigned int iterations = 10;

    auto task = double_coro_loop(a, b, iterations, resume_count_a, resume_count_b);

    do {
        a.resume();
        b.resume();
    } while (!task.done());
    TEST_ASSERT_EQUAL_UINT(resume_count_a, iterations);
    TEST_ASSERT_EQUAL_UINT(resume_count_b, iterations);
}

void test_double_unordered_coroutine_blocking_patterns(void) {
    // Inttialize coroutine
    // Class to manage timer co-routines
    scheduler_unordered a;
    scheduler_unordered b;
    unsigned int resume_count_a{ 0 };
    unsigned int resume_count_b{ 0 };
    constexpr unsigned int iterations = 10;

    auto task = double_coro_loop(a, b, iterations, resume_count_a, resume_count_b);
    fprintf(stderr, "Task @ %p\n", (void*)&task);

    // Just resume a
    for (unsigned int i = 0; i < iterations; i++) {
        a.resume();
    }
    // Should only iterate once
    TEST_ASSERT_EQUAL_UINT(resume_count_a, 1);
    TEST_ASSERT_EQUAL_UINT(resume_count_b, 0);

    // Just resume b
    for (unsigned int i = 0; i < iterations; i++) {
        b.resume();
    }
    // Should only iterate once
    TEST_ASSERT_EQUAL_UINT(resume_count_a, 1);
    TEST_ASSERT_EQUAL_UINT(resume_count_b, 1);

    // Redundant resumes
    for (unsigned int i = 0; i < 4; i++) {
        a.resume();
        a.resume();
        b.resume();
        b.resume();
    }
    // Should only iterate once
    TEST_ASSERT_EQUAL_UINT(resume_count_a, 5);
    TEST_ASSERT_EQUAL_UINT(resume_count_b, 5);

    a.resume();
    a.resume();
    b.resume();
    b.resume();

    TEST_ASSERT_EQUAL_UINT(resume_count_a, 6);
    TEST_ASSERT_EQUAL_UINT(resume_count_b, 6);

    // Close out
    do {
        a.resume();
        b.resume();
    } while (!task.done());
    TEST_ASSERT_EQUAL_UINT(resume_count_a, iterations);
    TEST_ASSERT_EQUAL_UINT(resume_count_b, iterations);
}

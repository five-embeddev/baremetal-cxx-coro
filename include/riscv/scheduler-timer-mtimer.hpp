/*
   Timer configuration to schedule coutines.

   The timer is a riscv machine hardware timer.

   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef SCHEDULER_TIMER_MTIMER_HPP
#define SCHEDULER_TIMER_MTIMER_HPP

#include <chrono>

#include "timer.hpp"

// Traits for the scheduler
// This should mimic std::chrono::steady_clock
struct mtimer_clock {
    using duration = std::chrono::steady_clock::duration;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<mtimer_clock>;
    static constexpr bool is_steady = true;

    static time_point now() noexcept {
        driver::timer<> mtimer;
        time_point epoch(duration::zero());
        auto duration_since_epoch = mtimer.get_time<duration>();
        auto retval = epoch + duration_since_epoch;
        return retval;
    }
};


#endif

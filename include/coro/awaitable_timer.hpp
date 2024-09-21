/*
   Create an awaitable concept for use with the simple co-routine scheduler

   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef AWAITABLE_TIMER_HPP
#define AWAITABLE_TIMER_HPP

#include "scheduler.hpp"

#include <coroutine>
#include <chrono>

/* A class that implements the Awaitable concept.
   The template paramter is a scheduler.

   @tparam SCHEDULER The scheduler that will implement this delay.

*/
template<class SCHEDULER>
struct awaitable_timer {

    /** Create a timer with a given delay that can implment `co_await.
        @param scheduler  The object that will manage the execution of our co-routine.
        @param  delay     The time that the co-routine will be delayed for.
    */
    awaitable_timer(SCHEDULER& scheduler,
                    std::chrono::microseconds delay)
        : scheduler_{ scheduler }
        , delay_{ delay } {}

    bool await_ready() {
        // Returning true will execute immediately - Only wait if there is a delay.
        return delay_.count() == 0;
    }
    void await_suspend(std::coroutine_handle<> handle) {
        // Insert into the schedule.
        scheduler_.insert(handle, SCHEDULER::make_condition(delay_));
    }
    void await_resume() {
        // NOTE - At this point the member may have been clobered - dont' trust _delay..
    }

  private:
    SCHEDULER& scheduler_;
    const std::chrono::microseconds delay_;// Relative delay
};


/** Convinence structure to group a co-routine scheduler and delay.
 */
template<typename SCHEDULER, typename DELAY = std::chrono::microseconds>
struct scheduled_delay {
    SCHEDULER& scheduler;
    DELAY delay;
};

/** Allow a scheduler and  microseconds delay to be directly 'awaited' on.
 */
template<typename SCHEDULER, typename DELAY = std::chrono::microseconds>
auto operator co_await(scheduled_delay<SCHEDULER, DELAY>&& schedule_delay) {
    return awaitable_timer<SCHEDULER>{ schedule_delay.scheduler, schedule_delay.delay };
}

#endif// AWAITABLE_TIMER_HPP

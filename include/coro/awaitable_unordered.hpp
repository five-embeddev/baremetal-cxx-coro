/*
   Create an awaitable concept for use with the simple co-routine scheduler

   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef AWAITABLE_UNORDERED_HPP
#define AWAITABLE_UNORDERED_HPP

#include "scheduler.hpp"

#include <coroutine>
#include <chrono>

/* A class that implements the Awaitable concept.
   The template paramter is a scheduler.

   @tparam SCHEDULER The scheduler that will implement this delay.

*/
template<std::size_t MAX_TASKS>
struct awaitable_unordered {

    using SCHEDULER = scheduler_unordered<MAX_TASKS>;

    /** Create a unordered with a given delay that can implment `co_await.
        @param scheduler  The object that will manage the execution of our co-routine.
        @param  delay     The time that the co-routine will be delayed for.
    */
    awaitable_unordered(SCHEDULER& scheduler)
        : scheduler_{ scheduler } {
    }

    bool await_ready() noexcept(true) {
        // Wait until the explity context switch
        return false;
    }
    void await_suspend(std::coroutine_handle<> handle) noexcept(true) {
        // Insert into the schedule.
        scheduler_.insert(handle);// TODO - implicit
    }
    void await_resume() noexcept(true) {
        // NOTE - At this point the member may have been clobered - dont' trust _delay..
    }

  private:
    SCHEDULER& scheduler_;
};


/** Allow a scheduler and  microseconds delay to be directly 'awaited' on.
 */
template<std::size_t MAX_TASKS>
auto operator co_await(scheduler_unordered<MAX_TASKS>& scheduler) noexcept(true) {
    return awaitable_unordered{ scheduler };
}

#endif// AWAITABLE_UNORDERED_HPP

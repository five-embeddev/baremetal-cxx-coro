/*
   Create an awaitable concept for use with the simple co-routine scheduler

   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef AWAITABLE_PRIORITY_HPP
#define AWAITABLE_PRIORITY_HPP

#include "scheduler.hpp"

#include <coroutine>
#include <chrono>

/* A class that implements the Awaitable concept.
   The template paramter is a scheduler.

   @tparam SCHEDULER The scheduler that will implement this delay.

*/
template<class SCHEDULER>
struct awaitable_priority {

    /** Create a priority with a given delay that can implment `co_await.
        @param scheduler  The object that will manage the execution of our co-routine.
        @param  delay     The time that the co-routine will be delayed for.
    */
    awaitable_priority(SCHEDULER& scheduler,
                       int priority)
        : scheduler_{ scheduler }
        , priority_{ priority } {}

    bool await_ready() {
        // Wait until the explity context switch
        return false;
    }
    void await_suspend(std::coroutine_handle<> handle) {
        // Insert into the schedule.
        scheduler_.insert(handle, schedule_by_priority{ priority_ });// TODO - implicit
        // Update the schedule and resume any pending entries - but do not block.
        scheduler_.wakeup_pending();
    }
    void await_resume() {
        // NOTE - At this point the member may have been clobered - dont' trust _delay..
    }

  private:
    SCHEDULER& scheduler_;
    const int priority_;// Relative priority
};


/** Convinence structure to group a co-routine scheduler and delay.
 */
template<typename SCHEDULER>
struct scheduled_priority {
    SCHEDULER& scheduler;
    int priority;
};

/** Allow a scheduler and  microseconds delay to be directly 'awaited' on.
 */
template<typename SCHEDULER>
auto operator co_await(scheduled_priority<SCHEDULER>&& schedule_priority) {
    return awaitable_priority{ schedule_priority.scheduler, schedule_priority.priority };
}

#endif// AWAITABLE_PRIORITY_HPP

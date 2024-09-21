/*
   Schedule co-routines using the a generic schedule class passed via the template parameter.

   The generic type timer could be a std::chrono::steady_clock or it
   could be a hardware timer or priority level.

   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/


#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <coroutine>
#include <chrono>
#include <array>
#include <optional>

#include "../debug/trace.hpp"

#if defined(HOST_EMULATION)
#include <iostream>
#endif

#include "static_list.hpp"

using namespace std::literals::chrono_literals;

template<typename T>
concept HasWakeUpTest = requires(T t) {
    // The class has a method that will compare
    // A provided wakeup condition to the scheduled entries condition.
    // e.g.
    // Compare the current clock time to the scheduled wake up time
    // Compare the current interrupt mask to the scheduled mask
    // Compare the current priority level to the scheduled priority level
    t.ready_to_wake(t);
    // The class has a constant that indicates if a wakeup condition
    //{T::wake_is_exclusive()} -> std::same_as<bool>;
};


template<typename CLOCK_T>
class schedule_by_delay {
  public:
    using duration = typename CLOCK_T::duration;
    using time_point = typename CLOCK_T::time_point;

    static time_point now() {
        return CLOCK_T::now();
    }


    /** Schedule a coroutine to wakeup after delay microseconds.
     */
    explicit schedule_by_delay(std::chrono::microseconds delay)
        : expires_{ now() + duration_cast<duration>(delay) } {
    }

    /** Schedule a coroutine to wakeup immediately.
     */
    schedule_by_delay()
        : expires_{ now() } {
    }

    /** Compare to another condition, is this coroutine ready to wake.
     */
    bool ready_to_wake(const schedule_by_delay& current_state) const {
        return (current_state.expires_ > expires_);
    }

    /** Return the time to wait until this is ready.
     */
    typename CLOCK_T::duration delay(void) {
        auto t_now = now();
        if (expires_ > t_now) {
            return expires_ - t_now;
        }
        return 0s;
    }

  private:
    time_point expires_;// Absolute time point
};


/** Objects with the highest highest priority are scheduled to run first.
 */
class schedule_by_priority {
  public:
    explicit schedule_by_priority(int priority)
        : priority_{ priority } {
    }
    schedule_by_priority()
        : priority_{ 0 } {
    }
    /** Compare to another condition, is this coroutine ready to wake.
     */
    bool ready_to_wake(const schedule_by_priority& current_state) const {
        return (priority_ >= current_state.priority_);
    }

  private:
    int priority_;
};

/* A quick and dirty class to represent a coroutine that has been scheduled to run at a later time.

   This does NOT match any of the co-routine concepts.

   @tparam wake_condition A condition that will be used to wake the delayed co-routines.

 */
template<HasWakeUpTest WAKE_CONDITION_T>
class schedule_entry {

  public:
    /** Create a schedule entry with a given handle and time delta to
     * schedule the handle to wake up after
     * @param handle   C++ Co-routine handle to be scheduled.
     * @param delay    C++ microsecond delay
     */
    schedule_entry(std::coroutine_handle<> handle,
                   const WAKE_CONDITION_T& wake_condition)
        : handle_{ handle }
        , wake_condition_{ wake_condition } {}

    // Defaults
    schedule_entry() = delete;
    schedule_entry(const schedule_entry&) = delete;
    schedule_entry& operator=(const schedule_entry&) = delete;
    schedule_entry& operator=(schedule_entry&&) = delete;

    // Move constructor
    schedule_entry(schedule_entry&& src) {
        std::swap(handle_, src.handle_);
        std::swap(wake_condition_, src.wake_condition_);
    }

    /** Get the wake up condition for this co-routine
     */
    const WAKE_CONDITION_T& wake_condition(void) const {
        return wake_condition_;
    }

    /** If the time "now" is greater than the expiry time of this entry then
        resume he co-routine.
        @param  ready_condition Wake up if this condition is exceeded.
        @retval true   The co-routine was run
        @retval false  The co-routine was not run
     */
    bool ready_to_wake(const WAKE_CONDITION_T& ready_condition) {
        return wake_condition_.ready_to_wake(ready_condition);
    }

    std::coroutine_handle<> handle(void) const {
        // Context switch to co-routine
        return handle_;
    }

  private:
    std::coroutine_handle<> handle_;// Handle containing context to allow co-routine to resume.
    WAKE_CONDITION_T wake_condition_;
};


/* A quick and dirty class to act as a container for a set of scheduled co-routines.

   This does NOT match any of the co-routine concepts.

   @tparam wake_condition A condition that will be used to schedule the delayed co-routines. For example a clock.
   @tparam MAX_TASKS          A fixed array is used to schedule entries. This is the maximum number of entries.

 */
template<HasWakeUpTest WAKE_CONDITION_T,
         std::size_t MAX_TASKS = 10>
class scheduler_ordered {

  public:
    using CONDITION = WAKE_CONDITION_T;
    // Defaults
    scheduler_ordered() {}

    // The scheduler_ordered is intended to be instanciated once.
    scheduler_ordered(const scheduler_ordered&) = delete;
    scheduler_ordered(scheduler_ordered&&) = delete;
    scheduler_ordered& operator=(const scheduler_ordered&) = delete;
    scheduler_ordered& operator=(scheduler_ordered&&) = delete;

    /** Test for an empty schedule list .
        @retval true There are no co-routines scheduled to wake up.
     */
    bool empty() const noexcept {
        return waiting_.empty();
    }

    /** Create a condition for waking this type of scheduled object.
     */
    template<typename T>
    static WAKE_CONDITION_T make_condition(T& arg) {
        return WAKE_CONDITION_T{ arg };
    }

    /** Insert an entry to be scheduled to run after a given delay.

       @param handle            C++ Co-routine handle to be scheduled.
       @param wake_condition    Condition to wake on (such as clock time or priority)

    */
    void insert(std::coroutine_handle<> handle,
                const WAKE_CONDITION_T& wake_condition) {
        auto i = waiting_.begin();
        while (i != waiting_.end()) {
            if (wake_condition.ready_to_wake(i->wake_condition())) {
                break;
            }
            ++i;
        }
        waiting_.emplace(i, std::move(schedule_entry<WAKE_CONDITION_T>{ handle, wake_condition }));
    }


    /** Visit all pending coutines and see if they are ready to be executed.
        If not then calcuate the time to the next schedule co-routine and wait that long.

        Only block if the caller requests.

        Return true if there are still pending co-routines.

        @param ready_condition This condition is used to evaluate if a co-routine should wake.
        @retval (more routines are pending, next delay to wait)
    */
    std::pair<bool, std::optional<WAKE_CONDITION_T>>
        resume(const WAKE_CONDITION_T& ready_condition) {
        std::optional<WAKE_CONDITION_T> priority_condition;

        bool pending{ false };
        auto i = waiting_.begin();
        uint16_t j = 0;
        TRACE_VALUE(scheduler_update_i, j);
        TRACE_VALUE(scheduler_update_r, 0);
        while (i != waiting_.end()) {
            // We have seen at least one pending co-routine.
            pending = true;

            // Is this co-routine is due to run?
            if (i->ready_to_wake(ready_condition)) {
                std::optional<WAKE_CONDITION_T> c{ i->wake_condition() };
                auto handle{ i->handle() };
                waiting_.erase(i);

                // Don't continue iteration here, let the caller descide what to do.
                // It's quite possible something else was scheduled in the above call.

                // Return true so it calls us until all entries have
                // been visited and seen to be done..
                TRACE_VALUE_FLAG(scheduler_update_r, 1);
                handle.resume();
                return { true, priority_condition };// Does not return
            }
            else {
                // Keep track of the soonest scheduled co-routine.
                if (!priority_condition || priority_condition->ready_to_wake(i->wake_condition())) {
                    priority_condition = i->wake_condition();
                    TRACE_VALUE_FLAG(scheduler_update_r, 2);
                }
                else {
                    TRACE_VALUE_FLAG(scheduler_update_r, 4);
                }
            }
            ++i;
            TRACE_VALUE(scheduler_update_i, ++j);
        }
        // No entry is active, but there are pending entries and the caller requested we block.
        TRACE_VALUE_FLAG(scheduler_update_r, 8);
        return { pending, priority_condition };
    }

  private:
    //! Set of waiting tasks
    static_list<schedule_entry<WAKE_CONDITION_T>, MAX_TASKS> waiting_;
};

/** Scheduler for prioritized execution */
using scheduler_priority = scheduler_ordered<schedule_by_priority>;

/** Scheduler for delayed execution */
template<typename CLOCK_T>
using scheduler_delay = scheduler_ordered<schedule_by_delay<CLOCK_T>>;

/* A quick and dirty class to act as a container for a set of scheduled co-routines.

   This does NOT match any of the co-routine concepts.

   @tparam MAX_TASKS          A fixed array is used to schedule entries. This is the maximum number of entries.

 */
template<std::size_t MAX_TASKS = 10>
class scheduler_unordered {

  public:
    // Defaults
    scheduler_unordered() {}

    // The scheduler_unordered is intended to be instanciated once.
    scheduler_unordered(const scheduler_unordered&) = delete;
    scheduler_unordered(scheduler_unordered&&) = delete;
    scheduler_unordered& operator=(const scheduler_unordered&) = delete;
    scheduler_unordered& operator=(scheduler_unordered&&) = delete;

    /** Test for an empty schedule list .
        @retval true There are no co-routines scheduled to wake up.
     */
    bool empty() const noexcept {
        return waiting_.empty();
    }

    /* Insert an entry to be scheduled to run at a later point.

       @param handle            C++ Co-routine handle to be scheduled.

    */
    void insert(std::coroutine_handle<> handle) {
        waiting_.emplace_back(std::move(handle));
    }

    /* Wakeup the pending co-routine.
       @retval true  There are pending co-routines
       @retval false  There are no pending co-routines
     */
    void resume(void) {
        while (!waiting_.empty()) {
            auto handle{ *waiting_.begin() };
            waiting_.pop_front();
            handle.resume();
        }
    }

  private:
    //! Set of waiting tasks
    static_list<std::coroutine_handle<>, MAX_TASKS> waiting_;
};


#endif// SCHEDULER_HPP

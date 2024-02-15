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

    // static constexpr bool wake_is_exclusive() {
    //     return true;
    // }

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
        , done_{ false }
        , wake_condition_{ wake_condition } {}
    /** Create default schedule in the completed inactive state .
     */
    schedule_entry() {}

    // Defaults
    schedule_entry(const schedule_entry&) = delete;
    schedule_entry& operator=(const schedule_entry&) = delete;

    schedule_entry(schedule_entry&& src) {
        std::swap(handle_, src.handle_);
        std::swap(done_, src.done_);
        std::swap(wake_condition_, src.wake_condition_);
    }
    schedule_entry& operator=(schedule_entry&&) = delete;

    /** Overwrite an expired entry
     */
    void replace(std::coroutine_handle<> new_handle,
                 const WAKE_CONDITION_T& wake_condition) {
        done_ = false;
        handle_ = new_handle;
        wake_condition_ = wake_condition;
    }

    /** If the entry is done then it has already been scheduled and run.
        The caller is free to replace it.
     */
    bool is_done(void) const {
        return done_;
    }

    /** Get the wake up condition for this co-routine
     */
    const WAKE_CONDITION_T& wake_condition(void) const {
        return wake_condition_;
    }


    /** Get the expiry time for this co-routine
     * @retval true The alternative is higher priority.
     * @retval false This has higher pririty.
     */
    bool test_has_priority(const WAKE_CONDITION_T& alternative) const {
        return alternative.ready_to_wake(wake_condition_);
    }

    /** If the time "now" is greater than the expiry time of this entry then
        resume he co-routine.
        @param  ready_condition Wake up if this condition is exceeded.
        @retval true   The co-routine was run
        @retval false  The co-routine was not run
     */
    bool if_ready_then_resume(const WAKE_CONDITION_T& ready_condition) {
        if (!done_ && wake_condition_.ready_to_wake(ready_condition)) {
            // Flag it true and resume execution.
            done_ = true;
            // Context switch to co-routine
            handle_.resume();// << TO PENDING CO-ROUTINE <<
            // Does this reeturn ???
            return true;
        }
        return false;
    }

  private:
    std::coroutine_handle<> handle_;// Handle containing context to allow co-routine to resume.
    bool done_{ true };// When a co-routine is "done" the entry can be replaced.
    WAKE_CONDITION_T wake_condition_;
};


/* A quick and dirty class to act as a container for a set of scheduled co-routines.

   This does NOT match any of the co-routine concepts.

   @tparam wake_condition A condition that will be used to schedule the delayed co-routines. For example a clock.
   @tparam MAX_TASKS          A fixed array is used to schedule entries. This is the maximum number of entries.

 */
template<HasWakeUpTest WAKE_CONDITION_T,
         std::size_t MAX_TASKS = 10>
class scheduler {

  public:
    using CONDITION = WAKE_CONDITION_T;
    // Defaults
    scheduler() {}

    // The scheduler is intended to be instanciated once.
    scheduler(const scheduler&) = delete;
    scheduler(scheduler&&) = delete;
    scheduler& operator=(const scheduler&) = delete;
    scheduler& operator=(scheduler&&) = delete;


    template<typename T>
    static WAKE_CONDITION_T make_condition(T& arg) {
        return WAKE_CONDITION_T{ arg };
    }

    /* Insert an entry to be scheduled to run after a given delay.
       Either replace an entry that has expired and been run,  or
       append a new entry at the end of the active list

       @param handle            C++ Co-routine handle to be scheduled.
       @param wake_condition    Condition to wake on (such as clock time or priority)

    */
    void insert(std::coroutine_handle<> handle,
                const WAKE_CONDITION_T& wake_condition) {
        auto i = waiting_.begin();
        while (i != waiting_.end()) {
            if (i->test_has_priority(wake_condition)) {
                break;
            }
            ++i;
        }
        waiting_.emplace(i, std::move(schedule_entry<WAKE_CONDITION_T>{ handle, wake_condition }));

        // No spaxbre slots - append.
        //_waiting.push_back({h, delay});
        // TODO - ABORT
    }

    /* Wakeup the pending co-routine.
     */
    void wakeup_pending(void) {
        if (waiting_.empty()) {
            return;
        }
        const WAKE_CONDITION_T default_condition;
        // Is this co-routine is due to run?
        if (waiting_.begin()->if_ready_then_resume(default_condition)) {
            waiting_.pop_front();
            return;
        }
    }

    /* Visit all pending coutines and see if they are ready to be executed.
       If not then calcuate the time to the next schedule co-routine and wait that long.

       Only block if the caller requests.

       Return true if there are still pending co-routines.

       @retval (more routines are pending, next delay to wait)
    */
    std::pair<bool, std::optional<WAKE_CONDITION_T>>
        update(const WAKE_CONDITION_T& ready_condition) {
        std::optional<WAKE_CONDITION_T> priority_condition;

        bool pending{ false };
        auto i = waiting_.begin();
        while (i != waiting_.end()) {
            // We have seen at least one pending co-routine.
            pending = true;

            // Is this co-routine is due to run?
            if (i->if_ready_then_resume(ready_condition)) {
                std::optional<WAKE_CONDITION_T> c{ i->wake_condition() };
                waiting_.erase(i);

                // Don't continue iteration here, let the caller descide what to do.
                // It's quite possible something else was scheduled in the above call.

                // Return true so it calls us until all entries have
                // been visited and seen to be done..
                priority_condition.reset();
                return { true, c };
            }
            else {
                // Keep track of the soonest scheduled co-routine.
                if (!priority_condition || i->test_has_priority(*priority_condition)) {
                    priority_condition = i->wake_condition();
                }
            }
            ++i;
        }
        // No entry is active, but there are pending entries and the caller requested we block.
        return { pending, priority_condition };
    }

  private:
    //! Set of waiting tasks
    static_list<schedule_entry<WAKE_CONDITION_T>, MAX_TASKS> waiting_;
};

#endif// SCHEDULER_HPP

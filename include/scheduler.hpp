/*
   Schedule coutines using the timer object passed via the template parameter.

   The timer could be std::chrono::steady_clock or it could be a hardware timer.

   SPDX-License-Identifier: Unlicense

   NOT TESTED. DO NOT USE.
   
*/

#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <coroutine>
#include <chrono>
#include <array>

using namespace std::literals::chrono_literals;

/* A quick and dirty class to represent a coroutine that has been scheduled to run at a later time.

   This does NOT match any of the co-routine concepts.

   @tparam clock A clock that will be used to schedule the delayed co-routines.

 */
template <typename clock>
class schedule_entry {
public:
    using time_point = std::chrono::time_point<clock>;

    /** Create a schedule entry with a given handle and time delta to
     * schedule the handle to wake up after
     * @param handle   C++ Co-routine handle to be scheduled.
     * @param delay    C++ microsecond delay
     */
    schedule_entry(std::coroutine_handle<> handle, 
                   std::chrono::microseconds delay)  
        : handle_(handle)
        , expires_(clock::now() + delay) 
        , done_{false}
        {}

    /** Create default schedule in the completed inactive state .
     */
    schedule_entry() {}

    // Defaults
    schedule_entry(const schedule_entry&) = delete;
    schedule_entry& operator=(const schedule_entry&) = delete;

    schedule_entry(schedule_entry&& src) {
        std::swap(handle_, src.handle_);
        std::swap(done_, src.done_);
        std::swap(expires_, src.expires_);
    }
    schedule_entry& operator=(schedule_entry&&) = delete;

    /** Overwrite an expired entry 
    */
    void replace(std::coroutine_handle<> new_handle, 
                 std::chrono::microseconds delay)  {
        done_ = false;
        handle_ = new_handle;
        expires_ = clock::now() + delay;
    }

    /** If the entry is done then it has already been scheduled and run.
        The caller is free to replace it.
     */
    bool is_done(void) const {
        return done_;
    }

    /** Get the expiry time for this co-routine
     */
    time_point expires(void) const {
        return expires_;
    }

    /** If the time "now" is greater than the expiry time of this entry then 
        resume he co-routine.
        @retval true   The co-routine was run
        @retval false  The co-routine was not run
     */
    bool if_expired_then_resume(const time_point &now) {
        if (now > expires_) {
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
    std::coroutine_handle<> handle_; // Handle containing context to allow co-routine to resume.
    time_point expires_; // Absolute time point
    bool done_{true}; // When a co-routine is "done" the entry can be replaced.
};


/* A quick and dirty class to act as a container for a set of scheduled co-routines.
   
   This does NOT match any of the co-routine concepts.

   @tparam clock      A clock that will be used to schedule the delayed co-routines.
   @tparam MAX_TASKS  A fixed array is used to schedule entries. This is the maximum number of entries.

 */
template <typename clock, 
          std::size_t MAX_TASKS=10>
class scheduler {

public:
    using time_point = std::chrono::time_point<clock>;

    // Defaults
    scheduler() = default;
    scheduler(const scheduler&) = delete;
    scheduler(scheduler&&) = delete;
    scheduler& operator=(const scheduler&) = delete;
    scheduler& operator=(scheduler&&) = delete;


    /* Insert an entry to be scheduled to run after a given delay.
       Either replace an entry that has expired and been run,  or
       append a new entry at the end of the active list

       @param handle   C++ Co-routine handle to be scheduled.
       @param delay    C++ microsecond delay

    */
    void insert(std::coroutine_handle<> handle, 
                std::chrono::microseconds delay) {
        for (auto &i : waiting_) {
            if (i.is_done()) {
                // Re-use this slot.
                i.replace(handle, delay);
                return;
            }
        }
        // No spare slots - append.
        //_waiting.push_back({h, delay});
        // TODO - ABORT
    }

    /* Visit all pending coutines and see if they are ready to be executed.
       If not then calcuate the time to the next schedule co-routine and wait that long.
       
       Only block if the caller requests.

       Return true if there are still pending co-routines.

       @retval (more routines are pending, next delay to wait)
    */
    std::pair<bool, typename clock::duration> update(void) {
        time_point now {clock::now()};
        time_point expires = now;
        bool have_expires = false;
        bool pending = false;
        for (auto &i : waiting_) {
            // The list can contain expired slots, ignore them.
            if (i.is_done()) continue;
            // We have seen at least one pending co-routine.
            pending = true;
            
            // Is this co-routine is due to run?
            if (i.if_expired_then_resume(now)) {

                // Don't continue iteration here, let the caller descide what to do.
                // It's quite possible something else was scheduled in the above call.

                // Return true so it calls us until all entries have
                // been visited and seen to be done..

                return {true, clock::duration::zero()};
            } else {
                // Keep track of the soonest scheduled co-routine.
                if  (!have_expires)  {
                    have_expires = true;
                    expires = i.expires();
                } else if (i.expires() < expires) {
                    have_expires = true;
                    expires = i.expires();
                }
            }
        }
        if (pending && have_expires) {
            // No entry is active, but there are pending entries and the caller requested we block.
            auto diff = expires - now;
            return {pending, diff};
        } else {
            return {pending, clock::duration::zero()};
        }
    }
private:
    std::array<schedule_entry<clock>, MAX_TASKS> waiting_;
};

#endif // SCHEDULER_HPP

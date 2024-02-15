/*
   Simple host emulation for machine mode timer driver for RISC-V standard timer.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef TIMER_HPP
#define TIMER_HPP

#include <cstdint>
#include <chrono>

namespace driver {

    /** Simple TIMER driver class
     */
    template<class BASE_DURATION = std::chrono::microseconds>
    class timer {
      public:
        timer()
            : start_{ std::chrono::high_resolution_clock::now() } {
        }

        /** Duration of each timer tick */
        using timer_ticks = std::chrono::microseconds;

        /** Set the timer compare point using a std::chrono::duration timer offset
         */
        template<class T = BASE_DURATION>
        void set_time_cmp(T time_offset) {
            set_ticks_time_cmp(std::chrono::duration_cast<timer_ticks>(time_offset));
        }
        /** Get the system timer as a std::chrono::duration value
         */
        template<class T = BASE_DURATION>
        T get_time(void) {
            return std::chrono::duration_cast<T>(get_ticks_time());
        }
        /** Set the time compare point in ticks of the system timer counter.
         */
        void set_ticks_time_cmp(timer_ticks time_offset) {
            mtimecmp_ = time_offset;
        }
        /** Return the current system time as a duration since the mtime counter was initialized
         */
        timer_ticks get_ticks_time(void) {
            return std::chrono::duration_cast<timer_ticks>(std::chrono::high_resolution_clock::now() - start_);
        }

      private:
        timer_ticks mtimecmp_;
        const std::chrono::time_point<std::chrono::high_resolution_clock> start_;
    };

}// namespace driver

#endif// #ifdef TIMER_HPP

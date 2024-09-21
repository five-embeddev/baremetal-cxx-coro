/*
   Quick and dirty tracer.

   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/


#ifndef TRACE_HPP
#define TRACE_HPP

#define ENABLE_TRACE

#ifndef ENABLE_TRACE

#define TRACE_TIMESTAMP(timestamp)
#define TRACE_VALUE(label, value)
#define TRACE_VALUE_FLAG(label, mask)

#else

#include <stdint.h>

#define TRACE_ENTRY_COUNT_POW2 128

using trace_timestamp_t = uint32_t;

struct trace_log_entry {
    uint32_t index;
    trace_timestamp_t timestamp;
    trace_timestamp_t delta_timestamp;
    bool coro_pending;
    trace_timestamp_t next_wake_delay;
    uint16_t scheduler_update_i;
    uint16_t scheduler_update_r;
};

class trace_log_manager {
    static constexpr uint32_t COUNT_MASK = TRACE_ENTRY_COUNT_POW2 - 1;

  public:
    static void next_entry(trace_timestamp_t t) {
        auto prev_timestamp = records[_index].timestamp;
        _index = (_index + 1) & COUNT_MASK;
        std::fill((uint8_t*)&records[_index],// cppcheck-suppress mismatchingContainers
                  (uint8_t*)&records[_index + 1],
                  0U);
        records[_index].index = _index;
        records[_index].timestamp = t;
        records[_index].delta_timestamp = t - prev_timestamp;
    }
    static trace_log_entry& get_entry(void) {
        return records[_index];
    }

  private:
    inline static trace_log_entry records[TRACE_ENTRY_COUNT_POW2]{};
    inline static uint32_t _index{ COUNT_MASK };
};


#define TRACE_TIMESTAMP(timestamp) \
    trace_log_manager::next_entry(timestamp)

#define TRACE_VALUE(label, VALUE) \
    trace_log_manager::get_entry().label = (VALUE)

#define TRACE_VALUE_FLAG(label, VALUE) \
    trace_log_manager::get_entry().label |= (VALUE)

#endif


#endif

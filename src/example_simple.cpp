/*
   Baremetal example program with co-routines driven by timer interrupt.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#include <cstdint>
#include <chrono>

#include "example_simple.hpp"

#include "embeddev_coro.hpp"
#include "embeddev_riscv.hpp"

#include "debug/trace.hpp"

static volatile uint64_t timestamp_simple{ 0 };
static volatile uint32_t resume_simple{ 0 };


/**  A simple task to schedule
 * @tparam SCHEDULER    The type of scheduler that will manage this co-routine's execution.
 * @param scheduler     The actual of scheduler that will manage this co-routine's execution.
 * @param period        This co-routine will periodically wake up with this period.
 * @param resume_count  Count the number of times this co-routine wakes up. For introspection only.
 */
template<typename SCHEDULER>
nop_task resuming_on_delay(
    SCHEDULER& scheduler,
    std::chrono::microseconds period,
    volatile uint32_t& resume_count) {
    for (auto i = 0; i < 10; i++) {
        co_await scheduled_delay{ scheduler, period };
        resume_count = i + 1;
    }
}


void example_simple(riscv_cpu_t& core) {

    // Timer driver
    driver::timer<> mtimer;

    scheduler_delay<mtimer_clock> scheduler;

    // Global interrupt disable
    riscv::csrs.mstatus.mie.clr();

    timestamp_simple = mtimer.get_time<driver::timer<>::timer_ticks>().count();
    // Timer will fire immediately
    mtimer.set_time_cmp(mtimer_clock::duration::zero());

    // Run two concurrent loops. The first loop wil run concurrently to the second loop.
    auto t = resuming_on_delay(scheduler, 100ms, resume_simple);
    (void)t;

    // The periodic interrupt lambda function.
    // The context (drivers etc) is captured via reference using [&]
    static const auto handler = [&](void) {
        auto this_cause = riscv::csrs.mcause.read();
        if (this_cause & riscv::csr::mcause_data::interrupt::BIT_MASK) {
            this_cause &= 0xFF;
            // Known exceptions
            switch (this_cause) {
            case riscv::interrupts::mti:
                timestamp_simple = mtimer.get_time<driver::timer<>::timer_ticks>().count();
                // Timer interrupt disable
                riscv::csrs.mie.mti.clr();
                break;
            }
        }
    };
    // Install the above lambda function as the machine mode IRQ handler.
    riscv::irq::handler irq_handler(handler);

    // Busy loop
    do {
        // Get a delay to the next co-routine wakup
        schedule_by_delay<mtimer_clock> now;
        auto [pending, next_wake] = scheduler.resume(now);
        if (pending) {
            // Next wakeup
            mtimer.set_time_cmp(next_wake->delay());
            // Timer interrupt enable
            riscv::csrs.mstatus.mie.clr();
            riscv::csrs.mie.mti.set();
            // WFI Should be called while interrupts are disabled
            // to ensure interrupt enable and WFI is atomic.
            core.wfi();
            riscv::csrs.mstatus.mie.set();
        }
    } while (true);
}

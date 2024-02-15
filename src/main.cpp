/*
   Baremetal main program with co-routines drivem by timer interrupt.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/


#include <cstdint>
#include <coroutine>
#include <chrono>

#if defined(HOST_EMULATION)
#include "host/riscv-cpu.hpp"
#include "host/riscv-isa.hpp"
#include "host/riscv-csr.hpp"
#include "riscv/riscv-interrupts.hpp"
#include "host/riscv-irq.hpp"
#include "host/timer.hpp"
#else
// RISC-V CSR definitions and access classes
// Download: wget https://raw.githubusercontent.com/five-embeddev/riscv-csr-access/master/include/riscv-csr.hpp
#include "riscv/riscv-isa.hpp"
#include "riscv/riscv-csr.hpp"
#include "riscv/riscv-interrupts.hpp"
#include "riscv/riscv-irq.hpp"
#include "riscv/timer.hpp"
#include "riscv/scheduler-timer-mtimer.hpp"
#endif

#include "coro/scheduler.hpp"
#include "coro/nop_task.hpp"
#include "coro/awaitable_timer.hpp"

#if defined(HOST_EMULATION)
using mtimer_clock = std::chrono::steady_clock;
#endif

// Global to hold current timestamp
static volatile uint64_t timestamp{ 0 };
static volatile uint32_t resume_count_100{ 0 };
static volatile uint32_t resume_count_200{ 0 };

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


int main(int argc, const char** argv) {
    // Timer driver
    driver::timer<> mtimer;
#if defined(HOST_EMULATION)
    riscv::cpu core{ argc, argv, riscv::csrs, mtimer };
#else
    (void)argc;
    (void)argv;
    riscv::cpu core;
#endif

    // Class to manage timer co-routines
    scheduler<schedule_by_delay<mtimer_clock>> mtimer_coro_scheduler;

    // Global interrupt disable
    riscv::csrs.mstatus.mie.clr();


    timestamp = mtimer.get_time<driver::timer<>::timer_ticks>().count();
    // Timer will fire immediately
    mtimer.set_time_cmp(mtimer_clock::duration::zero());

    // Test coro

    // Run two concurrent loops. The first loop wil run concurrently to the second loop.
    auto t0 = resuming_on_delay(mtimer_coro_scheduler, 100ms, resume_count_100);
    (void)t0;
    // The second loop will finish later than the first loop.
    auto t1 = resuming_on_delay(mtimer_coro_scheduler, 200ms, resume_count_200);
    (void)t1;
    // As we are not in a task, at this point the loops have not completed,
    // but have placed work in the schedule.

    // The periodic interrupt lambda function.
    // The context (drivers etc) is captured via reference using [&]
    static const auto handler = [&](void) {
        auto this_cause = riscv::csrs.mcause.read();
        if (this_cause & riscv::csr::mcause_data::interrupt::BIT_MASK) {
            this_cause &= 0xFF;
            // Known exceptions
            switch (this_cause) {
            case riscv::interrupts::mti:
                timestamp = mtimer.get_time<driver::timer<>::timer_ticks>().count();
                // Timer interrupt disable
                riscv::csrs.mie.mti.clr();
                break;
            }
        }
    };
    // Install the above lambda function as the machine mode IRQ handler.
    riscv::irq::handler irq_handler(handler);


    // Timer interrupt enable
    riscv::csrs.mie.mti.set();
    // Global interrupt enable
    riscv::csrs.mstatus.mie.set();


    // Busy loop
    do {
        // Get a delay to the next co-routine wakup
        schedule_by_delay<mtimer_clock> now;
        auto [pending, next_wake] = mtimer_coro_scheduler.update(now);
        if (pending) {
            // Next wakeup
            mtimer.set_time_cmp(next_wake->delay());
            // Timer interrupt enable
            riscv::csrs.mie.mti.set();
        }
        core.wfi();
    } while (true);

    return 0;
}

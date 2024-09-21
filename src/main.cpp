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
#include "coro/awaitable_unordered.hpp"

#include "debug/trace.hpp"

#if defined(HOST_EMULATION)
using mtimer_clock = std::chrono::steady_clock;
#endif

constexpr bool TEST_SIMPLE = true;
constexpr bool TEST_COMPLEX = false;

// Global to hold current timestamp
static volatile uint64_t timestamp{ 0 };
static volatile uint32_t resume_count{ 0 };
static volatile uint32_t resume_count_100{ 0 };
static volatile uint32_t resume_count_200{ 0 };

static volatile uint32_t resume_main_t3{ 0 };
static volatile uint32_t resume_main_t4{ 0 };
static volatile uint32_t resume_main_t5{ 0 };
static volatile uint32_t resume_isr_t3{ 0 };
static volatile uint32_t resume_isr_t4{ 0 };
static volatile uint32_t resume_isr_t5{ 0 };


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


/**  A simple task to schedule in ISR and main thread
 * @param isr_scheduler     The actual of scheduler that will manage this co-routine's execution.
 * @param main_scheduler     The actual of scheduler that will manage this co-routine's execution.
 * @param isr_count          Count the number of times this co-routine wakes up in the ISR. For introspection only.
 * @param main_count         Count the number of times this co-routine wakes up in main(). For introspection only.
 */
template<typename ISR_SCHEDULER, typename MAIN_SCHEDULER>
nop_task resuming_on_isr_and_main(
    ISR_SCHEDULER& isr_scheduler,
    MAIN_SCHEDULER& main_scheduler,
    volatile uint32_t& isr_resume_count,
    volatile uint32_t& main_resume_count) {
    uint32_t i{ 0 };
    while (true) {
        i++;
        co_await isr_scheduler;
        isr_resume_count = i;
        co_await main_scheduler;
        main_resume_count = i;
    }
}

int main(int argc, const char** argv) {
#if defined(HOST_EMULATION)
    riscv::cpu core{ argc, argv, riscv::csrs, mtimer };
#else
    (void)argc;
    (void)argv;
    riscv::cpu core;
#endif

    if (TEST_SIMPLE) {
        // Timer driver
        driver::timer<> mtimer;

        scheduler_delay<mtimer_clock> scheduler;

        // Global interrupt disable
        riscv::csrs.mstatus.mie.clr();

        timestamp = mtimer.get_time<driver::timer<>::timer_ticks>().count();
        // Timer will fire immediately
        mtimer.set_time_cmp(mtimer_clock::duration::zero());

        // Run two concurrent loops. The first loop wil run concurrently to the second loop.
        auto t = resuming_on_delay(scheduler, 100ms, resume_count);
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
                    timestamp = mtimer.get_time<driver::timer<>::timer_ticks>().count();
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

        // Done
        return 0;
    }


    if (TEST_COMPLEX) {
        // Timer driver
        driver::timer<> mtimer;

        // Class to manage timer co-routines
        scheduler_delay<mtimer_clock> mtimer_coro_scheduler;
        scheduler_unordered<1> isr_context;
        scheduler_unordered<1> isr_mti_context;
        scheduler_unordered<1> isr_mei_context;
        scheduler_unordered<3> main_thread;

        // Global interrupt disable
        riscv::csrs.mstatus.mie.clr();


        timestamp = mtimer.get_time<driver::timer<>::timer_ticks>().count();
        // Timer will fire immediately
        mtimer.set_time_cmp(mtimer_clock::duration::zero());

        // Test coro

        // Run two concurrent loops. The first loop wil run concurrently to the second loop.
        auto t0 = resuming_on_delay(mtimer_coro_scheduler, 10s, resume_count_100);
        (void)t0;
        // The second loop will finish later than the first loop.
        auto t1 = resuming_on_delay(mtimer_coro_scheduler, 20s, resume_count_200);
        (void)t1;
        // As we are not in a task, at this point the loops have not completed,
        // but have placed work in the schedule.

        // Run in background, wake up on all ISRs and main
        auto t3 = resuming_on_isr_and_main(isr_context, main_thread, resume_isr_t3, resume_main_t3);
        (void)t3;
        // Run in background, wake up on all Timer ISRs and main
        auto t4 = resuming_on_isr_and_main(isr_mti_context, main_thread, resume_isr_t4, resume_main_t4);
        (void)t4;
        // Run in background, wake up on all External ISRs and main
        auto t5 = resuming_on_isr_and_main(isr_mei_context, main_thread, resume_isr_t5, resume_main_t5);
        (void)t5;


        // The periodic interrupt lambda function.
        // The context (drivers etc) is captured via reference using [&]
        static const auto handler = [&](void) {
            isr_context.resume();
            auto this_cause = riscv::csrs.mcause.read();
            if (this_cause & riscv::csr::mcause_data::interrupt::BIT_MASK) {
                this_cause &= 0xFF;
                // Known exceptions
                switch (this_cause) {
                case riscv::interrupts::mei:
                    isr_mei_context.resume();
                    break;
                case riscv::interrupts::mti:
                    timestamp = mtimer.get_time<driver::timer<>::timer_ticks>().count();
                    // Timer interrupt disable
                    riscv::csrs.mie.mti.clr();
                    isr_mti_context.resume();
                    break;
                }
            }

            ;

            // schedule_by_delay<mtimer_clock> now;
            // auto [pending, next_wake] = mtimer_coro_scheduler.update(now);
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
            TRACE_TIMESTAMP(mtimer.get_time<driver::timer<>::timer_ticks>().count());
            auto [pending, next_wake] = mtimer_coro_scheduler.resume(now);
            TRACE_VALUE(coro_pending, pending);
            TRACE_VALUE(next_wake_delay, next_wake->delay().count());
            // Wakeup any co-routines waiting for main thread processing.
            main_thread.resume();
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

        return 0;
    }
}

/*
   Baremetal example program with co-routines driven by timer interrupt.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#include <cstdint>

#include "example_irq.hpp"

#include "embeddev_coro.hpp"
#include "embeddev_riscv.hpp"

static volatile uint64_t timestamp_irq{ 0 };
static volatile uint32_t resume_main_t3{ 0 };
static volatile uint32_t resume_main_t4{ 0 };
static volatile uint32_t resume_main_t5{ 0 };
static volatile uint32_t resume_isr_t3{ 0 };
static volatile uint32_t resume_isr_t4{ 0 };
static volatile uint32_t resume_isr_t5{ 0 };

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

void example_irq(riscv_cpu_t& core) {
    // Timer driver
    driver::timer<> mtimer;

    // Global interrupt disable
    riscv::csrs.mstatus.mie.clr();

    timestamp_irq = mtimer.get_time<driver::timer<>::timer_ticks>().count();
    // Timer will fire immediately
    mtimer.set_time_cmp(mtimer_clock::duration::zero());

    scheduler_unordered<1> isr_context;
    scheduler_unordered<1> isr_mti_context;
    scheduler_unordered<1> isr_mei_context;
    scheduler_unordered<3> main_thread;

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
                timestamp_irq = mtimer.get_time<driver::timer<>::timer_ticks>().count();
                // Timer interrupt disable
                riscv::csrs.mie.mti.clr();
                isr_mti_context.resume();
                break;
            }
        }

        ;
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
        // Wakeup any co-routines waiting for main thread processing.
        main_thread.resume();
        // Next wakeup
        mtimer.set_time_cmp(100us);
        // Timer interrupt enable
        riscv::csrs.mstatus.mie.clr();
        riscv::csrs.mie.mti.set();
        // WFI Should be called while interrupts are disabled
        // to ensure interrupt enable and WFI is atomic.
        core.wfi();
        riscv::csrs.mstatus.mie.set();
    } while (true);
}

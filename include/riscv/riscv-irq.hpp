/*
   RISC-V machine interrupts.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef RISCV_IRQ_HPP
#define RISCV_IRQ_HPP

#include "riscv-csr.hpp"

#include <cstdint>

namespace riscv {

    // Place the IRQ related code in a seperate namespace
    namespace irq {
        // Machine mode interrupt service routine
        // Defined as an interrupt function to ensure correct 'mret' exit is generated.
        static void entry(void) __attribute__((interrupt("machine")));

        /** IRQ Handler class. Allows a lambda function (or other function
         * object) to be registered as the machine mode IRQ hander.
         */
        class handler {
          public:
            /** Create an IRQ handler class to install a
                function as the machine mode irq handler */
            template<class T>
            handler(T const& isr_handler);

            // Boilerplate delete defaults - non copyable class
            handler(const handler&) = delete;
            handler& operator=(const handler&) = delete;
            handler(handler&&) = delete;
            handler& operator=(handler&&) = delete;

          private:
            static inline void (*_execute_handler)(void);
            // Trampoline function is required to bridge from the entry point
            // function declared with specific attributes and alignments to this class member.
            friend void entry(void);
            static inline void handler_entry(void) {
                _execute_handler();
            }
        };
    }// namespace irq

    // Implement the IRQ handler
    namespace irq {

        // IRQ handler constructor
        // This is defined as a template to prevent dynamic memory allocation
        // by ensuring code can be generated according to the lambda function type defined in main.
        // That ensures a std::function() does not need to be used as a generic function call interface.
        template<class T>
        handler::handler(T const& isr_handler) {
            // This will call the C++ function object method that represents the lamda function above.
            // This is required to provide the context of the function call that is captured by the lambda.
            // A RISC-V optimization uses the MSCRATCH register to hold the function object context pointer.
            _execute_handler = [](void) {
                // Read the context from the interrupt scratch register.
                uintptr_t isr_context = riscv::csrs.mscratch.read();
                // Call into the lambda function.
                return ((T*)isr_context)->operator()();
            };
            // Get a pointer to the IRQ context and save in the interrupt scratch register.
            uintptr_t isr_context = (uintptr_t)&isr_handler;
            riscv::csrs.mscratch.write(reinterpret_cast<std::uintptr_t>(isr_context));
            // Write the entry() function to the mtvec register to install our IRQ handler.
            riscv::csrs.mtvec.write(reinterpret_cast<std::uintptr_t>(entry));
        }

#pragma GCC push_options
// Force the alignment for mtvec.BASE.
// A 'xC' extension program could be aligned to to bytes.
#pragma GCC optimize("align-functions=4")
        static void entry(void) {
            // Jump into the function defined within the irq::handler class.
            handler::handler_entry();
        }
#pragma GCC pop_options
    }// namespace irq

}// namespace riscv


#endif /* RISCV_IRQ_HPP */

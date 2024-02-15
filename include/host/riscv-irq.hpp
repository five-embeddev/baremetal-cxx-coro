/*
   RISC-V machine interrupts host emulation.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef RISCV_IRQ_HPP
#define RISCV_IRQ_HPP

#include "riscv-csr.hpp"

#include <functional>

namespace riscv {

    // Place the IRQ related code in a seperate namespace
    namespace irq {

        /** IRQ Handler class. Allows a lambda function (or other function
         * object) to be registered as the machine mode IRQ hander.
         */
        class handler {
          public:
            handler(std::function<void(void)>&& isr_handler)
                : irq_callback{ isr_handler } {
            }

            // Boilerplate delete defaults - non copyable class
            handler(const handler&) = delete;
            handler& operator=(const handler&) = delete;
            handler(handler&&) = delete;
            handler& operator=(handler&&) = delete;

            const std::function<void(void)> irq_callback;
        };
    }// namespace irq

}// namespace riscv


#endif /* RISCV_IRQ_HPP */

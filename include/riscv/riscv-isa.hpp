/*
   RISC-V Instructions
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef RISCV_ISA_HPP
#define RISCV_ISA_HPP


namespace riscv {

    struct cpu {
        void wfi() const {
            __asm__ volatile("wfi");
        }
    };

}// namespace riscv


#endif

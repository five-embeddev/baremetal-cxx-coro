/*
   RISC-V CPU Host Emulation.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef RISCV_CPU_HPP
#define RISCV_CPU_HPP

namespace riscv {

    template<class CSR_T, class TIMER_T>
    class cpu {
      public:
        cpu(int argc, const char** argv, CSR_T& csrs, TIMER_T& timer) {
            (void)argc;
            (void)argv;
            (void)csrs;
            (void)timer;
        }
        void wfi() {
        }
    };

}// namespace riscv

#endif

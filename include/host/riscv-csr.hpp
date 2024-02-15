/*
   Host emulation register access classes for RISC-V system registers.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef RISCV_CSR_HPP
#define RISCV_CSR_HPP

#include <cstdint>

namespace riscv {

    using uint_xlen_t = std::uint32_t;

    namespace csr {
        namespace mcause_data {
            /** Parameter data for interrupt */
            struct interrupt {
                static constexpr uint_xlen_t BIT_MASK = (0x1UL << ((__riscv_xlen - 1)));
            };
        }// namespace mcause_data
    }// namespace csr


    class mtvec_emul {
      public:
        void write(uintptr_t vector) {
            vector_ = vector;
        }

      private:
        uintptr_t vector_;
    };

    class bit_emul {
      public:
        void set() {
            value_ = true;
        }
        void clr() {
            value_ = false;
        }

      private:
        bool value_;
    };

    class mie_emul {
      public:
        bit_emul mti;
    };

    class mstatus_emul {
      public:
        bit_emul mie;
    };

    class mcause_emul {
      public:
        unsigned int read() {
            return 0;
        }
    };


    struct {
        mcause_emul mcause;
        mtvec_emul mtvec;
        mie_emul mie;
        mstatus_emul mstatus;
    } csrs;

};// namespace riscv


#endif

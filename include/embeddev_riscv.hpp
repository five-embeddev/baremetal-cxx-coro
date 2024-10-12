/*
   Baremetal example C++ RISC-V support library.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef EMBEDDEV_RISCV_H_
#define EMBEDDEV_RISCV_H_

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

#if defined(HOST_EMULATION)
using mtimer_clock = std::chrono::steady_clock;
#endif

#if defined(HOST_EMULATION)
using riscv_cpu_t = riscv::cpu<riscv::csr_s, driver::timer<>>;
#else
using riscv_cpu_t = riscv::cpu;
#endif

#endif// EMBEDDEV_RISCV_H_

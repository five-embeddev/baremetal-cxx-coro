/*
   Baremetal example program with co-routines driven by timer interrupt.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef EXAMPLE_IRQ_H_
#define EXAMPLE_IRQ_H_

#include "embeddev_riscv.hpp"

void example_irq(riscv_cpu_t& core);

#endif// EXAMPLE_IRQ_H_

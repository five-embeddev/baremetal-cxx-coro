/*
   Baremetal example program with co-routines driven by timer interrupt.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef EXAMPLE_TIMER_H_
#define EXAMPLE_TIMER_H_

#include "embeddev_riscv.hpp"

void example_timer(riscv_cpu_t& core);

#endif// EXAMPLE_TIMER_H_

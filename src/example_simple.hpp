/*
   Baremetal example program with co-routines driven by timer interrupt.
   SPDX-License-Identifier: Unlicense

   https://five-embeddev.com/

*/

#ifndef EXAMPLE_SIMPLE_H_
#define EXAMPLE_SIMPLE_H_

#include "embeddev_riscv.hpp"

void example_simple(riscv_cpu_t& core);

#endif// EXAMPLE_SIMPLE_H_

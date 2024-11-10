#!/usr/bin/env bash
docker run \
    --interactive \
    --rm \
    -v .:/project \
    -e DECODE_ELF=build_target/src/main.elf \
    fiveembeddev/riscv_gtkwave_base:latest \
    /opt/riscv_gtkwave/bin/decode_addr |  c++filt

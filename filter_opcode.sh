#!/usr/bin/env sh
docker run \
    --interactive \
    --rm \
    fiveembeddev/riscv_gtkwave_base:latest \
    /opt/riscv_gtkwave/bin/decode_inst-rv32imac

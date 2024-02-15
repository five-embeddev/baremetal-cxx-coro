# Run the Cmake build with Qemu
qemu-system-riscv32 \
    -nographic \
    -machine sifive_e \
    -d unimp,guest_errors \
    -gdb tcp::51234 \
    -S

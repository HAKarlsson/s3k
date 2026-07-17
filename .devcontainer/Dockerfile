FROM debian:13

RUN apt-get update && apt-get install -y \
    build-essential curl git \
    gcc-riscv64-unknown-elf \
    binutils-riscv64-unknown-elf \
    picolibc-riscv64-unknown-elf \
    meson \
    gdb-multiarch \
    qemu-system-riscv

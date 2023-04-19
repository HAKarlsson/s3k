### Kernel configuration
# build dir
BUILD_DIR?=build
OBJ_DIR?=$(BUILD_DIR)/obj
# config
CONFIG_H?=config.h
# Platform
PLATFORM?=plat/virt
# Prefix for toolchain
RISCV_PREFIX?=riscv64-unknown-elf-

ARCH=rv64imaczicsr
ABI=lp64
CMODEL=medany

### Compilation configuration
CC=${RISCV_PREFIX}gcc
SIZE=${RISCV_PREFIX}size
OBJDUMP=${RISCV_PREFIX}objdump

INC =-include${PLATFORM}/platform.h -include ${CONFIG_H} -Iinc -I${PLATFORM}
CFLAGS =-march=$(ARCH) -mabi=$(ABI) -mcmodel=$(CMODEL)\
	-std=c11 \
	-Wall -Wextra -Werror \
	-Wno-unused-parameter \
	-Wshadow -fno-common \
	-ffunction-sections -fdata-sections\
	-ffreestanding\
	-Wno-builtin-declaration-mismatch\
	-flto -fwhole-program\
	--specs=nosys.specs\
	-Wstack-usage=1024 -fstack-usage\
	-fno-stack-protector \
	-Os -g3
ASFLAGS=-march=$(ARCH) -mabi=$(ABI) -mcmodel=$(CMODEL)\
	-g3
LDFLAGS=-march=$(ARCH) -mabi=$(ABI) -mcmodel=$(CMODEL)\
	-nostartfiles -ffreestanding\
	-Wstack-usage=1024 -fstack-usage\
	-fno-stack-protector \
	-Wl,--gc-sections\
	-flto\
	-T${PLATFORM}/linker.ld

#	-Wl,--no-warn-rwx-segments\ <== this option is not supported by all linkers

# See LICENSE file for copyright and license details.
.POSIX:

# Program name
PROGRAM?=s3k

# Kernel configuration
CONFIG_H?=../config.def.h

# Platform
PLATFORM?=../plat/virt

# Common
COMMON = ../common

# Drivers
DRIVERS = ../drivers

# Build directory
BUILD?=build

# Prefix for toolchain
RISCV_PREFIX?=riscv64-unknown-elf-

# Platform specific config
include ${PLATFORM}/platform.mk

### Tools
CC=${RISCV_PREFIX}gcc
SIZE=${RISCV_PREFIX}size
OBJDUMP=${RISCV_PREFIX}objdump
OBJCOPY=${RISCV_PREFIX}objcopy

vpath %.c src
vpath %.S src

# Source files
SRCS=head.S trap.S cap_operations.c cap_table.c cap_utils.c csr.c \
     current.c exception.c info.c kassert.c preemption.c proc.c \
     schedule.c syscall.c wfi.c ${COMMON}/altio.c
SRCS+=${PLATFORM_SRCS}

# Compilation flags
CFLAGS =-march=${ARCH} -mabi=${ABI} -mcmodel=${CMODEL}
CFLAGS+=-std=c11
CFLAGS+=-Wall -Wextra -Werror
CFLAGS+=-Wno-unused-parameter
CFLAGS+=-Wshadow -fno-common
CFLAGS+=-Wno-builtin-declaration-mismatch
CFLAGS+=-Os -g3
CFLAGS+=-nostartfiles -ffreestanding
CFLAGS+=-Wstack-usage=256 -fstack-usage
CFLAGS+=-fno-stack-protector
CFLAGS+=-Wl,--gc-sections
CFLAGS+=-flto -fwhole-program
CFLAGS+=--specs=nosys.specs
CFLAGS+=-ffunction-sections -fdata-sections
CFLAGS+=-T${PLATFORM}/platform.ld -Tlinker.ld


INC+=-include ${PLATFORM}/config.h
INC+=-include ${CONFIG_H}
INC+=-Iinc 
INC+=-I${DRIVERS}/inc
INC+=-I${COMMON}/inc

ELF=${BUILD}/${PROGRAM}.elf
DEP=${BUILD}/${PROGRAM}.d
BIN=${BUILD}/${PROGRAM}.bin
DA=${BUILD}/${PROGRAM}.da

all: options kernel dasm size

options:
	@printf "build options:\n"
	@printf "CC       = ${CC}\n"
	@printf "CFLAGS   = ${CFLAGS}\n"
	@printf "INC      = ${INC}\n"
	@printf "PLATFORM = ${PLATFORM}\n"
	@printf "CONFIG_H = ${abspath ${CONFIG_H}}\n"
	@printf "SRCS     = ${SRCS}\n"
	@printf "BUILD    = ${abspath ${BUILD}}\n"

elf: ${ELF}

bin: ${BIN}

dasm: ${DA}

size: ${ELF}
	${SIZE} $<

kernel: ${ELF}

format:
	clang-format -i ${shell find -wholename "*.[chC]" -not -path '*/.*'}

clean:
	rm -f ${ELF} ${DEP} ${BIN} ${DA}

${BUILD}:
	mkdir -p $@

${BUILD}/%.elf: ${SRCS} | ${BUILD}
	${CC} ${CFLAGS} ${INC} -MMD -o $@ ${filter %.S, $^} ${filter %.c, $^}

${BUILD}/%.bin: ${OBJS}
	${OBJCOPY} -O binary $< $@

${BUILD}/%.da: ${BUILD}/%.elf
	${OBJDUMP} -d $< > $@

.PHONY: all options clean dasm docs test kernel size

-include ${DEP}

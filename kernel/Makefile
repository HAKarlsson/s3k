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

# Search path
vpath %.c src ${COMMON}/src
vpath %.S src

# Source files
SRCS=head.S trap.S
SRCS+=altio/altio.c kassert.c
SRCS+=cap/table.c cap/types.c
SRCS+=proc/proc.c kernel.c
SRCS+=exception.c
SRCS+=syscall/syscall.c syscall/util.c
SRCS+=sched/sched.c sched/wfi.c sched/preemption.c
SRCS+=csr.c

SRCS+=${PLATFORM_SRCS}

OBJS=${patsubst %.c, ${BUILD}/%.o, ${filter %.c, ${SRCS}}} \
	${patsubst %.S, ${BUILD}/%.o, ${filter %.S, ${SRCS}}}
DEPS=${patsubst %, ${BUILD}/%.d, ${SRCS}}

# Compilation flags
CFLAGS =-march=${ARCH} -mabi=${ABI} -mcmodel=${CMODEL}
CFLAGS+=-std=c11
CFLAGS+=-Wall -Wextra -Werror -Wconversion
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
INC+=-Iinc -I${COMMON}/inc

ELF=${BUILD}/${PROGRAM}.elf
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
	rm -rf ${BUILD}

${BUILD}/%.o: %.S
	@mkdir -p ${@D}
	${CC} ${CFLAGS} ${INC} -MMD -c -o $@ $<

${BUILD}/%.o: %.c
	@mkdir -p ${@D}
	${CC} ${CFLAGS} ${INC} -MMD -c -o $@ $<

${ELF}: ${OBJS}
	@mkdir -p ${@D}
	${CC} ${CFLAGS} ${INC} -o $@ ${OBJS}

${BIN}: ${ELF}
	@mkdir -p ${@D}
	${OBJCOPY} -O binary $< $@

${BUILD}/%.da: ${BUILD}/%.elf
	@mkdir -p ${@D}
	${OBJDUMP} -d $< > $@

.PHONY: all options clean dasm docs test kernel size

-include ${DEPS}

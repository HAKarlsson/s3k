# See LICENSE file for copyright and license details.
.POSIX:

# Program name
PROGRAM?=s3k

# Kernel configuration
CONFIG_H?=config.h

# Platform
PLATFORM?=virt

# Build directory
BUILD?=build

# Prefix for toolchain
RISCV_PREFIX?=riscv64-unknown-elf-

### Tools
CC=${RISCV_PREFIX}gcc
SIZE=${RISCV_PREFIX}size
OBJDUMP=${RISCV_PREFIX}objdump
OBJCOPY=${RISCV_PREFIX}objcopy

# Compilation flags
ARCH=rv64imac_zicsr_zifencei
ABI=lp64
CMODEL=medany

INC =-include plat/${PLATFORM}/platform.h -include ${CONFIG_H} -Iinc -Iplat/${PLATFORM}
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
CFLAGS+=-Tplat/${PLATFORM}/linker.ld
CFLAGS+=-ffunction-sections -fdata-sections

# Sources
vpath %.c src
vpath %.S src

SRCS=head.S trap.S altio.c cap_operations.c cap_table.c cap_utils.c csr.c \
     current.c exception.c info.c ipc.c kassert.c preemption.c proc.c \
     schedule.c syscall.c wfi.c

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
	@printf "BUILD    = ${abspath ${BUILD}}\n"

elf: ${ELF}

bin: ${BIN}

dasm: ${DA}

size: ${ELF}
	${SIZE} $<

format:
	clang-format -i ${shell find -wholename "*.[chC]" -not -path '*/.*'}

clean:
	rm -f ${ELF} ${DEP} ${BIN} ${DA}

${BUILD}:
	mkdir -p $@

${BUILD}/%.elf: ${SRCS} | ${BUILD}
	${CC} ${CFLAGS} ${INC} -MMD -o $@ ${filter-out %.h, $^}

${BUILD}/%.bin: ${OBJS}
	${OBJCOPY} -O binary $< $@

${BUILD}/%.da: ${BUILD}/%.elf
	${OBJDUMP} -S $< > $@

.PHONY: all options clean dasm docs test kernel size

-include ${DEP}

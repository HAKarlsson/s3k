# See LICENSE file for copyright and license details.
.POSIX:

# Program name
PROGRAM?=s3k

# Kernel configuration
CONFIG_H?=config.h

# Platform
PLATFORM?=plat/virt

# Build directory
BUILD?=build

# Prefix for toolchain
RISCV_PREFIX?=riscv64-unknown-elf-

### Tools
CC=${RISCV_PREFIX}gcc
SIZE=${RISCV_PREFIX}size
OBJDUMP=${RISCV_PREFIX}objdump

# Compilation flags
ARCH=rv64imaczicsr
ABI=lp64
CMODEL=medany

INC =-include ${PLATFORM}/platform.h -include ${CONFIG_H} -Iinc -I${PLATFORM}
CFLAGS =-march=${ARCH} -mabi=${ABI} -mcmodel=${CMODEL} \
        -std=c11 \
	-Wall -Wextra -Werror \
	-Wno-unused-parameter \
	-Wshadow -fno-common \
	-ffunction-sections -fdata-sections \
	-ffreestanding \
	-Wno-builtin-declaration-mismatch \
	-flto -fwhole-program \
	--specs=nosys.specs \
	-Wstack-usage=1024 -fstack-usage \
	-fno-stack-protector \
	-Os -g3

ASFLAGS=-march=${ARCH} -mabi=${ABI} -mcmodel=${CMODEL} \
	-g3
LDFLAGS=-march=${ARCH} -mabi=${ABI} -mcmodel=${CMODEL} \
	-nostartfiles -ffreestanding \
	-Wstack-usage=1024 -fstack-usage \
	-fno-stack-protector \
	-Wl,--gc-sections \
	-flto \
	-T${PLATFORM}/linker.ld

# Sources
vpath %.c src
vpath %.S src

SRCS=head.S trap.S cnode.c csr.c current.c exception.c proc.c \
     schedule.c syscall.c timer.c wfi.c altio.c kassert.c preemption.c
OBJS=${patsubst %.S, ${BUILD}/%.o, ${patsubst %.c, ${BUILD}/%.o, ${SRCS}}}
DEPS=${OBJS:.o=.d}

ELF=${BUILD}/${PROGRAM}.elf
DA=${BUILD}/${PROGRAM}.da

all: options kernel dasm size

options:
	@printf "build options:\n"
	@printf "CC        = ${CC}\n"
	@printf "LDFLAGS   = ${LDFLAGS}\n"
	@printf "ASFLAGS   = ${ASFLAGS}\n"
	@printf "CFLAGS    = ${CFLAGS}\n"
	@printf "INC       = ${INC}\n"
	@printf "CONFIG_H  = ${abspath ${CONFIG_H}}\n"
	@printf "BUILD     = ${abspath ${BUILD}}\n"

kernel: ${ELF}

dasm: ${DA}

size: ${ELF}
	@printf "SIZE ${@F}\n"
	@${SIZE} $<

format:
	clang-format -i ${shell find -wholename "*.[chC]" -not -path '*/.*'}

clean:
	rm -rf ${BUILD}

${BUILD}:
	mkdir -p $@

${BUILD}/%.o: %.S | ${BUILD}
	@printf "CC ${@F}\n"
	@${CC} ${ASFLAGS} ${INC} -MMD -c -o $@ $<

${BUILD}/%.o: %.c | ${BUILD}
	@printf "CC ${@F}\n"
	@${CC} ${CFLAGS} ${INC} -MMD -c -o $@ $<

${BUILD}/%.elf: ${OBJS}
	@printf "CC ${@F}\n"
	@${CC} ${LDFLAGS} -o $@ $^

${BUILD}/%.da: ${BUILD}/%.elf
	@printf "OBJDUMP ${@F}\n"
	@${OBJDUMP} -S $< > $@

.PHONY: all options clean dasm docs test kernel size

-include ${DEPS}

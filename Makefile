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
CFLAGS =-march=${ARCH} -mabi=${ABI} -mcmodel=${CMODEL} \
        -std=c11 \
	-Wall -Wextra -Werror \
	-Wno-unused-parameter \
	-Wshadow -fno-common \
	-Wno-builtin-declaration-mismatch \
	-ffreestanding \
	-flto -fwhole-program \
	--specs=nosys.specs \
	-Wstack-usage=256 -fstack-usage \
	-fno-stack-protector \
	-Os -g3

ASFLAGS=-march=${ARCH} -mabi=${ABI} -mcmodel=${CMODEL} \
	-g3
LDFLAGS =-march=${ARCH} -mabi=${ABI} -mcmodel=${CMODEL} \
	-nostartfiles -ffreestanding \
	-Wstack-usage=256 -fstack-usage \
	-fno-stack-protector \
	-Wl,--gc-sections \
	-flto -fwhole-program \
	--specs=nosys.specs \
	-Tplat/${PLATFORM}/linker.ld \
	-ffunction-sections -fdata-sections \
	-fcall-saved-s1 -fcall-saved-s2 -fcall-saved-s3 -fcall-saved-s4 \
	-fcall-saved-s5 -fcall-saved-s6 -fcall-saved-s7 -fcall-saved-s8 \
	-fcall-saved-s9 -fcall-saved-s10 -fcall-saved-s11

# Sources
vpath %.c src
vpath %.S src

SRCS=head.S trap.S csr.c current.c exception.c proc.c \
     cap_table.c cap_operations.c cap_utils.c \
     info.c ipc.c schedule.c syscall.c wfi.c altio.c kassert.c preemption.c
OBJS=${patsubst %.S, ${BUILD}/%.o, ${patsubst %.c, ${BUILD}/%.o, ${SRCS}}}
DEPS=${OBJS:.o=.d}

ELF=${BUILD}/${PROGRAM}.elf
BIN=${BUILD}/${PROGRAM}.bin
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

elf: ${ELF}

bin: ${BIN}

dasm: ${DA}

size: ${ELF}
	${SIZE} $<

format:
	clang-format -i ${shell find -wholename "*.[chC]" -not -path '*/.*'}

clean:
	rm -rf ${BUILD}

${BUILD}:
	mkdir -p $@

${BUILD}/%.o: %.S | ${BUILD}
	${CC} ${ASFLAGS} ${INC} -MMD -c -o $@ $<

${BUILD}/%.o: %.c | ${BUILD}
	${CC} ${CFLAGS} ${INC} -MMD -c -o $@ $<

${BUILD}/%.elf: ${OBJS}
	${CC} ${LDFLAGS} -o $@ $^

${BUILD}/%.bin: ${OBJS}
	${OBJCOPY} -O binary $< $@

${BUILD}/%.da: ${BUILD}/%.elf
	${OBJDUMP} -S $< > $@

.PHONY: all options clean dasm docs test kernel size

-include ${DEPS}

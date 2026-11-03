#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
builddir=$(mktemp -d)
trap 'rm -f "$builddir/multicore" "$builddir/thread.o"; rmdir "$builddir"' EXIT
trap 'exit 1' HUP INT TERM

compiler=${CC:-riscv64-linux-gnu-gcc}
printf '#include "s3k/syscall.h"\n' | "$compiler" -std=gnu11 -Wall -Wextra -Werror \
	-fsyntax-only -I "$root/lib/include" -x c -
"$compiler" -std=gnu11 -O2 -Wall -Wextra -Werror -pthread \
	-c "$root/tests/thread.c" -o "$builddir/thread.o"
"$compiler" -std=gnu11 -O2 -Wall -Wextra -Werror -static -pthread \
	-I "$root/kern/include" -include "$root/tests/kernel-mocks.h" \
	-D_MAX_PID=4 -D_MAX_TIME_SLOT=32 -D_MAX_MEMORY_FUEL=16 \
	-D_MAX_TIME_FUEL=32 -D_MAX_MONITOR_FUEL=8 -D_MAX_IPC_FUEL=16 \
	-D_TIME_SLOT_US=1000 -D_MAX_PMP_SLOT=8 -D_NUM_MEMORY_CAPS=2 \
	-D_NUM_HARTS=2 -D_RTC_HZ=10000000 -D_CSPAD=0 \
	"$root/tests/multicore.c" "$root/kern/src/proc.c" "$root/kern/src/cap.c" \
	"$root/kern/src/mem.c" "$root/kern/src/tsl.c" "$root/kern/src/mon.c" \
	"$root/kern/src/ipc.c" "$root/kern/src/exception.c" \
	"$builddir/thread.o" \
	-o "$builddir/multicore"
"${RUNNER:-qemu-riscv64}" "$builddir/multicore"

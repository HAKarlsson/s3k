// See LICENSE file for copyright and license details.
#include "macro.inc"

.extern init_kernel
.extern schedule_next
.extern trap_entry
.extern trap_exit

.section .text.init

.globl _start
.type _start, @function
_start:
        csrw    mstatus,x0
        csrw    medeleg,x0
        csrw    mideleg,x0
        csrw    satp,x0
        // If hartid < MIN_HARTID, park it
        csrr    t0,mhartid
        li      t1,MIN_HARTID
        blt     t0,t1,_park

        // Load the global and stack pointers.
        .option push
        .option norelax
        la      gp, __global_pointer$
        .option pop
        load_sp t0

        // Set trap entry/handler.
        la   t0, trap_entry
        csrw mtvec, t0

        // Disable interrupts in machine mode.
        csrw mstatus, 0
        csrw mscratch,0

        // Enable timer interrupts for user mode.
        li   t0, (1<<7)
        csrw mie, t0

        // If hartid == MIN_HARTID, initialize the system, else wait.
        csrr    t0,mhartid
        li      t1,MIN_HARTID
        bne     t0,t1,_wait

        // write zeros to the bss section
        la t0, _bss
        la t1, _end
        j  2f
1:      sb zero, (t0)
        addi t0, t0, 1
2:      bne t0, t1, 1b

        // Initialize the kernel.
        call ctable_init
        call schedule_init
        call proc_init
        call info

_signal:
        fence
        li t0,1
        la t1,.done
        sw t0, 0(t1)

        j  1f

_wait:
        la t1,.done
        lw t0, 0(t1)
	wfi
        j _wait
        fence

1:
        // Start user processes.
        la   ra, trap_exit
        tail schedule

_park:
        wfi
        j _park

.section .data
.done:  .word 0

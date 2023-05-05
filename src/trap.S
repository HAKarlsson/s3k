// See LICENSE file for copyright and license details.


#include "macro.inc"

/** Offsets into PCB */
#define PC_OFFSET 0
#define RA_OFFSET 8
#define SP_OFFSET 16
#define GP_OFFSET 24
#define TP_OFFSET 32
#define T0_OFFSET 40
#define T1_OFFSET 48
#define T2_OFFSET 56
#define S0_OFFSET 64
#define S1_OFFSET 72
#define A0_OFFSET 80
#define A1_OFFSET 88
#define A2_OFFSET 96
#define A3_OFFSET 104
#define A4_OFFSET 112
#define A5_OFFSET 120
#define A6_OFFSET 128
#define A7_OFFSET 136
#define S2_OFFSET 144
#define S3_OFFSET 152
#define S4_OFFSET 160
#define S5_OFFSET 168
#define S6_OFFSET 176
#define S7_OFFSET 184
#define S8_OFFSET 192
#define S9_OFFSET 200
#define S10_OFFSET 208
#define S11_OFFSET 216
#define T3_OFFSET 224
#define T4_OFFSET 232
#define T5_OFFSET 240
#define T6_OFFSET 248

#define USER_ECALL 8
#define MSTATUS_MIE 8
#define SYSCALL_COUNT 10

.globl trap_entry
.globl trap_exit

.type trap_entry, @function
.type trap_exit, @function

.section .text.trap

        /* Function: trap_entry
        This is a trap handler that saves user registers to the
        process control block (PCB) and handles system calls and
        interrupts. It first saves the user thread pointer (TP) to the
        machine scratch register (mscratch) and loads the PCB for the
        current thread. If the PCB is zero, this means that the trap
        occurred in machine-mode, otherwise user-mode. If the trap
        occured in machine-mode, we load swap the tp with mscratch
        again, restoring the PCB pointer, and we call the timer
        interrupt handler (we assume that it is the only cause of a
        trap while in machine-mode).

        If the trap occured in user-mode, we save the user-registers
        to the PCB. This allows the kernel to resume the user at a
        later time.  Next, the handler checks the cause of the
        trap. If mcause < 0, we have a timer interrupt. If mcause ==
        USER_ECALL, then we have a system call. Otherwise we have a
        user-mode exception. For each types of trap, the handler jumps
        to the appropriate handler. The return address is set to
        trap_exit, so once trap has been handled, we resume a user
        process.
        */

.balign  16
trap_entry:
        /* save user tp to scratch, load PCB */
	csrrw tp, mscratch, tp

machine_interrupt:
        /* scratch == 0, then exception/interrupt in kernel */
	bnez  tp, save_context
        csrrw tp, mscratch, tp
        j     2f

save_context:
        /* save user registers (except tp and pc) */
	sd    ra, (RA_OFFSET)(tp)
	sd    sp, (SP_OFFSET)(tp)
	sd    gp, (GP_OFFSET)(tp)
	sd    t0, (T0_OFFSET)(tp)
	sd    t1, (T1_OFFSET)(tp)
	sd    t2, (T2_OFFSET)(tp)
	sd    s0, (S0_OFFSET)(tp)
	sd    s1, (S1_OFFSET)(tp)
	sd    a0, (A0_OFFSET)(tp)
	sd    a1, (A1_OFFSET)(tp)
	sd    a2, (A2_OFFSET)(tp)
	sd    a3, (A3_OFFSET)(tp)
	sd    a4, (A4_OFFSET)(tp)
	sd    a5, (A5_OFFSET)(tp)
	sd    a6, (A6_OFFSET)(tp)
	sd    a7, (A7_OFFSET)(tp)
	sd    s2, (S2_OFFSET)(tp)
	sd    s3, (S3_OFFSET)(tp)
	sd    s4, (S4_OFFSET)(tp)
	sd    s5, (S5_OFFSET)(tp)
	sd    s6, (S6_OFFSET)(tp)
	sd    s7, (S7_OFFSET)(tp)
	sd    s8, (S8_OFFSET)(tp)
	sd    s9, (S9_OFFSET)(tp)
	sd    s10,(S10_OFFSET)(tp)
	sd    s11,(S11_OFFSET)(tp)
	sd    t3, (T3_OFFSET)(tp)
	sd    t4, (T4_OFFSET)(tp)
	sd    t5, (T5_OFFSET)(tp)
	sd    t6, (T6_OFFSET)(tp)

        /* save user pc and tp */
	csrr  t1, mepc
	csrrw t2, mscratch, zero
	sd    t1, (PC_OFFSET)(tp)
	sd    t2, (TP_OFFSET)(tp)

2:      
        /* Load the global and stack pointer of the kernel. */
	.option push
	.option norelax
	la    gp, __global_pointer$
	.option pop
	load_sp t1

        /* Get the trap cause. */
	csrr  t1, mcause

        /* Set return address to trap_exit. */
        lla   ra, trap_exit

	/* If mcause < 0, then timer interrupt. */
	bltz  t1, schedule_yield

        /* Check if system call. */
	li    t2, USER_ECALL
	beq   t1, t2, syscall_handler

_exception:
        /* Otherwise, it is exception. */
        csrr  a0, mcause
        csrr  a1, mepc
        csrr  a2, mtval
        j     handle_exception

trap_exit:
        /* Set return to user-mode and enable preemption */
        csrw  mstatus, MSTATUS_MIE

	/* Load user pc to mepc. */
	ld    t0,(PC_OFFSET)(tp)
	csrw  mepc,t0

	/* Load other user registers */
	ld    ra, (RA_OFFSET)(tp)
	ld    sp, (SP_OFFSET)(tp)
	ld    gp, (GP_OFFSET)(tp)
	ld    t0, (T0_OFFSET)(tp)
	ld    t1, (T1_OFFSET)(tp)
	ld    t2, (T2_OFFSET)(tp)
	ld    s0, (S0_OFFSET)(tp)
	ld    s1, (S1_OFFSET)(tp)
	ld    a0, (A0_OFFSET)(tp)
	ld    a1, (A1_OFFSET)(tp)
	ld    a2, (A2_OFFSET)(tp)
	ld    a3, (A3_OFFSET)(tp)
	ld    a4, (A4_OFFSET)(tp)
	ld    a5, (A5_OFFSET)(tp)
	ld    a6, (A6_OFFSET)(tp)
	ld    a7, (A7_OFFSET)(tp)
	ld    s2, (S2_OFFSET)(tp)
	ld    s3, (S3_OFFSET)(tp)
	ld    s4, (S4_OFFSET)(tp)
	ld    s5, (S5_OFFSET)(tp)
	ld    s6, (S6_OFFSET)(tp)
	ld    s7, (S7_OFFSET)(tp)
	ld    s8, (S8_OFFSET)(tp)
	ld    s9, (S9_OFFSET)(tp)
	ld    s10,(S10_OFFSET)(tp)
	ld    s11,(S11_OFFSET)(tp)
	ld    t3, (T3_OFFSET)(tp)
	ld    t4, (T4_OFFSET)(tp)
	ld    t5, (T5_OFFSET)(tp)
	ld    t6, (T6_OFFSET)(tp)

	/* Disable preemption. */
	csrc  mstatus,MSTATUS_MIE

	/* Save the PCB pointer to mscratch. */
	csrw  mscratch,tp

	/* Load the user thread pointer. */
	ld    tp, (TP_OFFSET)(tp)

	/* Return to the user mode. */
	mret

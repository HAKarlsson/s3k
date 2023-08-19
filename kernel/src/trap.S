// See LICENSE file for copyright and license details.
#include "macro.inc"
#include "consts.h"

.globl trap_entry
.globl trap_syscall_exit
.globl trap_schedule_exit
.globl trap_exit

.type trap_entry, @function
.type trap_syscall_exit, @function
.type trap_schedule_exit, @function
.type trap_exit, @function

.section .text.trap
.balign 16
trap_entry:
	/* Save user tp to scratch, load PCB pointer */
	csrrw tp, mscratch, tp
	beqz tp, _myield
1:
	sd ra, REG_RA(tp)
	sd sp, REG_SP(tp)
	sd gp, REG_GP(tp)
	sd t0, REG_T0(tp)
	sd t1, REG_T1(tp)
	sd t2, REG_T2(tp)
	sd s0, REG_S0(tp)
	sd s1, REG_S1(tp)
	sd s2, REG_S2(tp)
	sd s3, REG_S3(tp)
	sd s4, REG_S4(tp)
	sd s5, REG_S5(tp)
	sd s6, REG_S6(tp)
	sd s7, REG_S7(tp)
	sd s8, REG_S8(tp)
	sd s9, REG_S9(tp)
	sd s10, REG_S10(tp)
	sd s11, REG_S11(tp)
	sd a0, REG_A0(tp)
	sd a1, REG_A1(tp)
	sd a2, REG_A2(tp)
	sd a3, REG_A3(tp)
	sd a4, REG_A4(tp)
	sd a5, REG_A5(tp)
	sd a6, REG_A6(tp)
	sd a7, REG_A7(tp)
	sd t3, REG_T3(tp)
	sd t4, REG_T4(tp)
	sd t5, REG_T5(tp)
	sd t6, REG_T6(tp)
	csrr t1, mepc
	sd t1, REG_PC(tp)
	csrrw t2, mscratch, 0
	sd t2, REG_TP(tp)

	/* Load the global and stack pointer of the kernel. */
	load_gp
	load_sp t2

	/* Check if system call. */
	csrr t2, mcause
	bltz t2, _yield

	la ra,trap_exit

	li t3, MCAUSE_USER_ECALL
	beq t2, t3, syscall_dispatch

_exception:
	/* Otherwise, it is exception. */
	mv a0, t1
	csrr a1, mepc
	csrr a2, mtval
	tail exception_handler

_myield:
	csrrw tp, mscratch, tp
	load_gp
	load_sp t1

_yield:
	/* Call scheduler */
	call schedule_yield

trap_exit:

	/* Load call-used registers, ra, sp, gp, tp, pc */
	ld t0, REG_PC(tp)
	csrw mepc,t0

	ld ra, REG_RA(tp)
	ld sp, REG_SP(tp)
	ld gp, REG_GP(tp)
	ld t0, REG_T0(tp)
	ld t1, REG_T1(tp)
	ld t2, REG_T2(tp)
	ld s0, REG_S0(tp)
	ld s1, REG_S1(tp)
	ld s2, REG_S2(tp)
	ld s3, REG_S3(tp)
	ld s4, REG_S4(tp)
	ld s5, REG_S5(tp)
	ld s6, REG_S6(tp)
	ld s7, REG_S7(tp)
	ld s8, REG_S8(tp)
	ld s9, REG_S9(tp)
	ld s10, REG_S10(tp)
	ld s11, REG_S11(tp)
	ld a0, REG_A0(tp)
	ld a1, REG_A1(tp)
	ld a2, REG_A2(tp)
	ld a3, REG_A3(tp)
	ld a4, REG_A4(tp)
	ld a5, REG_A5(tp)
	ld a6, REG_A6(tp)
	ld a7, REG_A7(tp)
	ld t3, REG_T3(tp)
	ld t4, REG_T4(tp)
	ld t5, REG_T5(tp)
	ld t6, REG_T6(tp)

	csrc mstatus,MSTATUS_MIE

	/* Save PCB pointer */
	csrw mscratch, tp

	/* Load user tp */
	ld tp, REG_TP(tp)

	mret

__hang:
	j __hang

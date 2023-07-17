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

	beqz tp, __hang

	/* Save call-used registers, ra, sp, gp, tp, pc */
	sd ra, REG_RA(tp)
	sd sp, REG_SP(tp)
	sd gp, REG_GP(tp)
	sd t0, REG_T0(tp)
	sd t1, REG_T1(tp)
	sd t2, REG_T2(tp)
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
	csrrw t1, mscratch, 0
	sd t1, REG_TP(tp)

	/* Load the global and stack pointer of the kernel. */
	load_gp
	load_sp t1

	/* Check if timer interrupt */
	csrr t1, mip
	andi t1, t1, MIP_MTIP
	bnez t1, _yield


	/* Check if system call. */
	csrr t1, mcause
	li t2, MCAUSE_USER_ECALL
	beq t1, t2, _syscall

_exception:
	/* Otherwise, it is exception. */
	mv a0, t1
	csrr a1, mepc
	csrr a2, mtval
	call exception_handler
	j trap_exit

_syscall:
	li t1, NUM_OF_SYSCALL
	bgeu t0, t1, _exception

#ifdef INSTRUMENTATION
	csrw mcycle, x0
#endif

1:	auipc ra, %pcrel_hi(_syscall_table)
	slli t0, t0, 2
	add ra, ra, t0
	jalr ra, %pcrel_lo(1b)(ra)

	/* If error code < 0, then yield */
	bltz a0, _yield

	/* Store error code */
	sd a0, REG_T0(tp)

	/* Advance PC */
	ld t1, REG_PC(tp)
	addi t1, t1, 4
	sd t1, REG_PC(tp)

#ifdef INSTRUMENTATION
	csrr t0,mcycle
	addi t1,tp,REG_INSTRUMENT_WCET
	amomax.d x0, t0, (t1)
#endif

	j trap_exit

_yield:
	/* Save call-saved registers */
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

	/* Call scheduler */
	call schedule_yield

	/* Load PMP addresses */
	ld s0, REG_PMPADDR0(tp)
	ld s1, REG_PMPADDR1(tp)
	ld s2, REG_PMPADDR2(tp)
	ld s3, REG_PMPADDR3(tp)
	ld s4, REG_PMPADDR4(tp)
	ld s5, REG_PMPADDR5(tp)
	ld s6, REG_PMPADDR6(tp)
	ld s7, REG_PMPADDR7(tp)
	csrw pmpaddr0, s0
	csrw pmpaddr1, s1
	csrw pmpaddr2, s2
	csrw pmpaddr3, s3
	csrw pmpaddr4, s4
	csrw pmpaddr5, s5
	csrw pmpaddr6, s6
	csrw pmpaddr7, s7

	/* Load PMP configuration */
	ld s0, REG_PMPCFG0(tp)
	csrw pmpcfg0, s0

	/* Load call-saved registers */
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

trap_exit:
	/* Check if timer interrupt */
	csrr t0, mip
	andi t0, t0, MIP_MTIP
	bnez t0, _yield

	/* Load call-used registers, ra, sp, gp, tp, pc */
	ld t0, REG_PC(tp)
	csrw mepc,t0

	ld ra, REG_RA(tp)
	ld sp, REG_SP(tp)
	ld gp, REG_GP(tp)
	ld t0, REG_T0(tp)
	ld t1, REG_T1(tp)
	ld t2, REG_T2(tp)
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

	/* Save PCB pointer */
	csrw mscratch, tp

	/* Load user tp */
	ld tp, REG_TP(tp)

	mret


.option push
.option norvc
_syscall_table:
	j syscall_get_info
	j syscall_get_reg
	j syscall_set_reg
	j syscall_yield
	j syscall_get_cap
	j syscall_move_cap
	j syscall_delete_cap
	j syscall_revoke_cap
	j syscall_derive_cap
	j syscall_pmp_set
	j syscall_pmp_clear
	j syscall_monitor_suspend
	j syscall_monitor_resume
	j syscall_monitor_get_reg
	j syscall_monitor_set_reg
	j syscall_monitor_get_cap
	j syscall_monitor_take_cap
	j syscall_monitor_give_cap
	j syscall_monitor_pmp_set
	j syscall_monitor_pmp_clear
	j syscall_socket_send
	j syscall_socket_recv
	j syscall_socket_sendrecv
.option pop

__hang:
	j __hang

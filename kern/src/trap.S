#include "asm_macro.h"		// Include assembly macros for register offsets and other utilities.

.extern exception_handler  	// External handler for exceptions.
.extern interrupt_handler  	// External handler for interrupts.
.extern syscall_handler    	// External handler for system calls.
.extern scheduler          	// External function for scheduling processes.

.globl trap_entry  		// Make trap_entry globally accessible.
.globl trap_exit   		// Make trap_exit globally accessible.
.globl trap_resume 		// Make trap_resume globally accessible.
.type  trap_entry, @function
.type  trap_exit, @function
.type  trap_resume, @function

.section .text

// Align the trap_entry function to a 16-byte boundary.
.balign 16
trap_entry:
	csrrw	a0,mscratch,a0		// Swap the current PCB pointer with mscratch.

	SREG	ra,PROC_RA(a0)		// Save return address.
	SREG	sp,PROC_SP(a0)		// Save stack pointer.
	SREG	gp,PROC_GP(a0)		// Save global pointer.
	SREG	tp,PROC_A0(a0)		// Save register a0.
	SREG	a1,PROC_A1(a0)		// Save register a1.
	SREG	a2,PROC_A2(a0)		// Save register a2.
	SREG	a3,PROC_A3(a0)		// Save register a3.
	SREG	a4,PROC_A4(a0)		// Save register a4.
	SREG	a5,PROC_A5(a0)		// Save register a5.
	SREG	a6,PROC_A6(a0)		// Save register a6.
	SREG	a7,PROC_A7(a0)		// Save register a7.
	SREG	t0,PROC_T0(a0)		// Save register t0.
	SREG	t1,PROC_T1(a0)		// Save register t1.
	SREG	t2,PROC_T2(a0)		// Save register t2.
	SREG	t3,PROC_T3(a0)		// Save register t3.
	SREG	t4,PROC_T4(a0)		// Save register t4.
	SREG	t5,PROC_T5(a0)		// Save register t5.
	SREG	t6,PROC_T6(a0)		// Save register t6.
	SREG	s0,PROC_S0(a0)		// Save register s0.
	SREG	s1,PROC_S1(a0)		// Save register s1.
	SREG	s2,PROC_S2(a0)		// Save register s2.
	SREG	s3,PROC_S3(a0)		// Save register s3.
	SREG	s4,PROC_S4(a0)		// Save register s4.
	SREG	s5,PROC_S5(a0)		// Save register s5.
	SREG	s6,PROC_S6(a0)		// Save register s6.
	SREG	s7,PROC_S7(a0)		// Save register s7.
	SREG	s8,PROC_S8(a0)		// Save register s8.
	SREG	s9,PROC_S9(a0)		// Save register s9.
	SREG	s10,PROC_S10(a0)	// Save register s10.
	SREG	s11,PROC_S11(a0)	// Save register s11.

	// Save program counter into the PCB.
	csrr	t0,mepc			// Load the program counter of the trapped instruction.
	SREG	t0,PROC_PC(a0)		// Save program counter.

	csrr	t0,mscratch		// Get user a0.
	SREG	t0,PROC_A0(a0)		// Save register a0.


	// Set up global and stack pointers for the kernel.
	.option push
	.option norelax
	la	gp,__global_pointer$	// Load the global pointer.
	.option pop
	la	sp,__stack_top		// Load the kernel stack top.

#ifdef SMP
	csrr	t0,mhartid
	slli	t0,t0,10
	add	sp,sp,t0
#endif

trap_dispatch:
	call	trap_handler		// Call the trap handler.

	beqz	a0,trap_idle		// If trap_handler returns 0, resume the process.

	csrr	s0,pmpcfg0		// Read current PMP configuration.
	bnez	s0,trap_exit		// If PMP is configured, skip PMP setup.

trap_resume:
	// Load PMP configuration from the PCB and set up PMP registers.
	LREG	s0,PROC_PMPCFG0(a0)	// Load PMP configuration.
	csrw	pmpcfg0,s0		// Write PMP configuration to pmpcfg0.

	LREG	s0,PROC_PMPADDR0(a0)	// Load PMP address0.
	LREG	s1,PROC_PMPADDR1(a0)	// Load PMP address1.
	LREG	s2,PROC_PMPADDR2(a0)	// Load PMP address2.
	LREG	s3,PROC_PMPADDR3(a0)	// Load PMP address3.
	LREG	s4,PROC_PMPADDR4(a0)	// Load PMP address4.
	LREG	s5,PROC_PMPADDR5(a0)	// Load PMP address5.
	LREG	s6,PROC_PMPADDR6(a0)	// Load PMP address6.
	LREG	s7,PROC_PMPADDR7(a0)	// Load PMP address7.
	csrw	pmpaddr0,s0		// Write PMP address to pmpaddr0.
	csrw	pmpaddr1,s1		// Write PMP address to pmpaddr1.
	csrw	pmpaddr2,s2		// Write PMP address to pmpaddr2.
	csrw	pmpaddr3,s3		// Write PMP address to pmpaddr3.
	csrw	pmpaddr4,s4		// Write PMP address to pmpaddr4.
	csrw	pmpaddr5,s5		// Write PMP address to pmpaddr5.
	csrw	pmpaddr6,s6		// Write PMP address to pmpaddr6.
	csrw	pmpaddr7,s7		// Write PMP address to pmpaddr7.


trap_exit:
	// Restore trap context and return to the next instruction.
	LREG	t0,PROC_PC(a0)		// Restore program counter.
	csrw	mepc,t0			// Write program counter back to mepc.

	LREG	t0,PROC_A0(a0)		// Save user a0 to mscratch.
	csrw	mscratch,t0
	LREG	ra,PROC_RA(a0)		// Restore return address.
	LREG	sp,PROC_SP(a0)		// Restore stack pointer.
	LREG	gp,PROC_GP(a0)		// Restore global pointer.
	LREG	tp,PROC_TP(a0)		// Restore register tp.
	LREG	a1,PROC_A1(a0)		// Restore register a1.
	LREG	a2,PROC_A2(a0)		// Restore register a2.
	LREG	a3,PROC_A3(a0)		// Restore register a3.
	LREG	a4,PROC_A4(a0)		// Restore register a4.
	LREG	a5,PROC_A5(a0)		// Restore register a5.
	LREG	a6,PROC_A6(a0)		// Restore register a6.
	LREG	a7,PROC_A7(a0)		// Restore register a7.
	LREG	t0,PROC_T0(a0)		// Restore register t0.
	LREG	t1,PROC_T1(a0)		// Restore register t1.
	LREG	t2,PROC_T2(a0)		// Restore register t2.
	LREG	t3,PROC_T3(a0)		// Restore register t3.
	LREG	t4,PROC_T4(a0)		// Restore register t4.
	LREG	t5,PROC_T5(a0)		// Restore register t5.
	LREG	t6,PROC_T6(a0)		// Restore register t6.
	LREG	s0,PROC_S0(a0)		// Restore register s0.
	LREG	s1,PROC_S1(a0)		// Restore register s1.
	LREG	s2,PROC_S2(a0)		// Restore register s2.
	LREG	s3,PROC_S3(a0)		// Restore register s3.
	LREG	s4,PROC_S4(a0)		// Restore register s4.
	LREG	s5,PROC_S5(a0)		// Restore register s5.
	LREG	s6,PROC_S6(a0)		// Restore register s6.
	LREG	s7,PROC_S7(a0)		// Restore register s7.
	LREG	s8,PROC_S8(a0)		// Restore register s8.
	LREG	s9,PROC_S9(a0)		// Restore register s9.
	LREG	s10,PROC_S10(a0)	// Restore register s10.
	LREG	s11,PROC_S11(a0)	// Restore register s11.

	csrrw	a0,mscratch,a0		// Swap PCB pointer with mscratch (user tp).
	mret				// Return from trap.

trap_idle:
	// If the trap handler returns 0, it means the process should be put to sleep.
	wfi				// Wait for interrupt to save power while idle.
	j trap_dispatch			// Loop back to trap_dispatch.

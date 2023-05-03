/* See LICENSE file for copyright and license details. */
#include "exception.h"

#include "current.h"
#include "preemption.h"
#include "proc.h"
#include "trap.h"

#define ILLEGAL_INSTRUCTION 0x2

#define MRET 0x30200073
#define SRET 0x10200073
#define URET 0x00200073

/**
 * This function restores the program counter and stack pointer to their values
 * prior to the exception, and clears the exception cause and exception value
 * registers.
 */
static void handle_ret(void)
{
	preemption_disable();
	current->regs.pc = current->regs.epc;
	current->regs.sp = current->regs.esp;
	current->regs.ecause = 0;
	current->regs.eval = 0;
	current->regs.epc = 0;
	current->regs.esp = 0;
	preemption_enable();
}

/*
 * This function is called when an exception occurs that doesn't fall under the
 * category of an illegal instruction return, such as a page fault or a timer
 * interrupt. It updates the exception cause, value, program counter, and stack
 * pointer in the process's registers, and switches to the trap handler program
 * counter and stack pointer.
 */
static void handle_default(uint64_t mcause, uint64_t mepc, uint64_t mtval)
{
	preemption_disable();
	current->regs.ecause = mcause;
	current->regs.eval = mtval;
	current->regs.epc = current->regs.pc;
	current->regs.esp = current->regs.sp;
	current->regs.pc = current->regs.tpc;
	current->regs.sp = current->regs.tsp;
	preemption_enable();
}

void handle_exception(uint64_t mcause, uint64_t mepc, uint64_t mtval)
{
	/* Check if it is a return from exception */
	if (mcause == ILLEGAL_INSTRUCTION
	    && (mtval == MRET || mtval == SRET || mtval == URET)) {
		// Handle return from exception
		handle_ret();
	} else {
		// Handle default exception
		handle_default(mcause, mepc, mtval);
	}
}

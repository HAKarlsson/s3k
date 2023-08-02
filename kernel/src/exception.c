/* See LICENSE file for copyright and license details. */
#include "exception.h"

#include "current.h"
#include "preemption.h"
#include "proc.h"

#define ILLEGAL_INSTRUCTION 0x2

#define MRET 0x30200073
#define SRET 0x10200073
#define URET 0x00200073

static void handle_ret(void);
static void handle_default(uint64_t mcause, uint64_t mepc, uint64_t mtval);

/**
 * This function restores the program counter and stack pointer to their values
 * prior to the exception, and clears the exception cause and exception value
 * registers.
 */
void handle_ret(void)
{
	current->regs[REG_PC] = current->regs[REG_EPC];
	current->regs[REG_SP] = current->regs[REG_ESP];
	current->regs[REG_ECAUSE] = 0;
	current->regs[REG_EVAL] = 0;
	current->regs[REG_EPC] = 0;
	current->regs[REG_ESP] = 0;
}

/*
 * This function is called when an exception occurs that doesn't fall under the
 * category of an illegal instruction return, such as a page fault or a timer
 * interrupt. It updates the exception cause, value, program counter, and stack
 * pointer in the process's registers, and switches to the trap handler program
 * counter and stack pointer.
 */
void handle_default(uint64_t mcause, uint64_t mepc, uint64_t mtval)
{
	current->regs[REG_ECAUSE] = mcause;
	current->regs[REG_EVAL] = mtval;
	current->regs[REG_EPC] = current->regs[REG_PC];
	current->regs[REG_ESP] = current->regs[REG_SP];
	current->regs[REG_PC] = current->regs[REG_TPC];
	current->regs[REG_SP] = current->regs[REG_TSP];
}

void exception_handler(uint64_t mcause, uint64_t mepc, uint64_t mtval)
{
	/* Check if it is a return from exception */
	if (mcause == ILLEGAL_INSTRUCTION && (mtval == MRET || mtval == SRET || mtval == URET)) {
		// Handle return from exception
		handle_ret();
	} else {
		// Handle default exception
		handle_default(mcause, mepc, mtval);
	}
}

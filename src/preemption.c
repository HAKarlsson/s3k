#include "preemption.h"

#include "csr.h"
#include "current.h"
#include "proc.h"

bool preemption(void)
{
	uint64_t time;
	__asm__ volatile("csrrw %0,mcycle,x0"
			 : "=r"(time));
	__asm__ volatile("amomax.d x0,%0,(%1)" ::"r"(time), "r"(&current->instrument_wcet));
	return csrr_mip() & MIP_MTIMER;
}

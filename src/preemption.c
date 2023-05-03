#include "preemption.h"

void preemption_disable(void)
{
	__asm__ volatile("csrci mstatus, 8");
}

void preemption_enable(void)
{
	__asm__ volatile("csrsi mstatus, 8");
}

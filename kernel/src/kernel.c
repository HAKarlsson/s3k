#include "kernel.h"

#include "altio/altio.h"
#include "cap/table.h"
#include "csr.h"
#include "drivers/uart.h"
#include "proc/proc.h"
#include "sched/sched.h"
#include "ticket_lock.h"

ticket_lock_t big_lock;

void kernel_init(void)
{
	tl_init(&big_lock);
	ctable_init();
	schedule_init();
	proc_init();
	uart_init();
	alt_puts("Kernel initialization complete");
}

uint64_t get_hartid()
{
	return csrr_mhartid();
}

void preempt_enable(void)
{
	__asm__ volatile("csrs mstatus,0x8");
}

void preempt_disable()
{
	__asm__ volatile("csrc mstatus,0x8");
}

void kernel_lock(void)
{
	preempt_disable();
	tl_acq(&big_lock);
}

void kernel_unlock(void)
{
	tl_rel(&big_lock);
	preempt_enable();
}

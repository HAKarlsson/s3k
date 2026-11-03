#include "interrupt.h"
#include "sched.h"

/**
 * Dummy interrupt handler.
 */
proc_result_t interrupt_handler(word_t cause, word_t tval)
{
	(void)cause;
	(void)tval;
	return sched_yield();
}

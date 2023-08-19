/* See LICENSE file for copyright and license details. */
#include "sched/wfi.h"

void wfi()
{
	__asm__ volatile("wfi");
}

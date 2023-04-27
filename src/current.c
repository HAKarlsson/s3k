#include "current.h"

proc_t *current __asm__("tp");

proc_t *current_get(void)
{
	return current;
}

void current_set(proc_t *proc)
{
	current = proc;
}

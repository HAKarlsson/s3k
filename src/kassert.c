#ifndef NDEBUG
#include "kassert.h"

#include "altio.h"

void kassert_failure(const char *file, unsigned long line, const char *expr)
{
	alt_printf("Assertion failed: %s, file %s, line 0x%X\n", expr, file,
		   line);
	while (1) {
		/* halt the core */
		__asm__ volatile("ebreak");
	}
}

#endif /* NDEBUG */

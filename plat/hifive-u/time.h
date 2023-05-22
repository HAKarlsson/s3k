#pragma once

#include <stdint.h>

#define MTIME ((volatile uint64_t *)0x200bff8ull)
#define MTIMECMP ((volatile uint64_t *)0x2004000ull)

static inline uint64_t time_get(void)
{
	return MTIME[0];
}

static inline void time_set(uint64_t time)
{
	MTIME[0] = time;
}

static inline uint64_t timeout_get(uint64_t hartid)
{
	return MTIMECMP[hartid];
}

static inline void timeout_set(uint64_t hartid, uint64_t timeout)
{
	MTIMECMP[hartid] = timeout;
}

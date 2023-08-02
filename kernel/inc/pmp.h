#pragma once

#include <stdint.h>

static inline void pmp_napot_decode(uint64_t addr, uint64_t *base, uint64_t *size)
{
	*base = ((addr + 1) & addr) << 2;
	*size = (((addr + 1) ^ addr) + 1) << 2;
}

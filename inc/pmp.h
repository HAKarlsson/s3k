#ifndef __PMP_H__
#define __PMP_H__

#include <stdint.h>

static inline uint64_t pmp_napot_begin(uint64_t addr)
{
	return ((addr + 1) & addr) << 2;
}

static inline uint64_t pmp_napot_end(uint64_t addr)
{
	return (((addr + 1) | addr) + 1) << 2;
}

#endif /* __PMP_H__ */

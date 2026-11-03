#pragma once

#include "csr.h"
#include "lock.h"

#define CSR_MIP_MTIP (1 << 7)

/**
 * Check if the current hart should preempt.
 */
static inline bool preempt(void)
{
	return csrr_mip() & CSR_MIP_MTIP;
}

static inline bool revoke_preempt(void)
{
	return preempt() || lock_contended();
}

#include "preemption.h"

#include "csr.h"

#define MIP_MTIP 0x80

bool preemption(void)
{
	return csrr_mip() & MIP_MTIP;
}

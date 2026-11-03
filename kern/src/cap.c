#include "cap.h"

#include "ipc.h"
#include "mem.h"
#include "mon.h"
#include "tsl.h"

int cap_transfer(pid_t owner, capty_t type, index_t i, pid_t target)
{
	switch (type) {
	case CAPTY_MEM:
		return mem_transfer(owner, i, target);
	case CAPTY_TSL:
		return tsl_transfer(owner, i, target);
	case CAPTY_MON:
		return mon_transfer(owner, i, target);
	case CAPTY_IPC:
		return ipc_transfer(owner, i, target);
	default:
		return ERR_INVALID_ARGUMENT;
	}
}

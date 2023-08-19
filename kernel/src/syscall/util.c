#include "cap/table.h"
#include "error.h"
#include "proc/current.h"
#include "proc/proc.h"
#include "sched/preemption.h"
#include "sched/sched.h"

#include <stdint.h>

bool check_preemption(void)
{
	if (!preemption())
		return false;
	current->regs.a0 = (uint64_t)PREEMPTED;
	return true;
}

bool check_invalid_cslot(uint64_t i)
{
	if (i < N_CAP)
		return false;
	current->regs.a0 = ERR_INVALID_CSLOT;
	return true;
}

bool check_src_empty(cidx_t src)
{
	if (!ctable_is_empty(src))
		return false;
	current->regs.a0 = ERR_SRC_EMPTY;
	return true;
}

bool check_dst_occupied(cidx_t src)
{
	if (ctable_is_empty(src))
		return false;
	current->regs.a0 = ERR_DST_OCCUPIED;
	return true;
}

bool check_invalid_derivation(cap_t src_cap, cap_t new_cap)
{
	if (cap_can_derive(src_cap, new_cap))
		return false;
	current->regs.a0 = ERR_INVALID_DERIVATION;
	return true;
}

bool check_invalid_capty(cap_t cap, capty_t ty)
{
	if (cap.type == ty)
		return false;
	current->regs.a0 = ERR_INVALID_CAPTY;
	return true;
}

bool check_invalid_pmpidx(uint64_t pmpidx)
{
	if (pmpidx < N_PMP)
		return false;
	current->regs.a0 = ERR_INVALID_PMPIDX;
	return true;
}

bool check_invalid_register(uint64_t reg)
{
	if (reg < N_REGS)
		return false;
	current->regs.a0 = ERR_INVALID_REGISTER;
	return true;
}

bool check_pmp_used(cap_t cap)
{
	if (!cap.pmp.used)
		return false;
	current->regs.a0 = ERR_PMP_USED;
	return true;
}

bool check_pmp_unused(cap_t cap)
{
	if (cap.pmp.used)
		return false;
	current->regs.a0 = ERR_PMP_UNUSED;
	return true;
}

bool check_pmp_occupied(uint64_t pid, uint64_t pmpidx)
{
	if (proc_pmp_is_empty(proc_get(pid), pmpidx))
		return false;
	current->regs.a0 = ERR_PMP_OCCUPIED;
	return true;
}

bool check_mon_invalid_pid(cap_t cap, uint64_t pid)
{
	if (cap.monitor.mark <= pid && pid < cap.monitor.end)
		return false;
	current->regs.a0 = ERR_MON_INVALID_PID;
	return true;
}

void delete_cap(cidx_t idx, cap_t cap)
{
	// Handle clean-up based on capability type
	switch (cap.type) {
	case CAPTY_TIME:
		schedule_delete(cap.time.hart, cap.time.mark, cap.time.end);
		break;
	case CAPTY_PMP:
		if (cap.pmp.used)
			proc_pmp_unload(proc_get(idx.pid), cap.pmp.index);
		break;
	}
	ctable_delete(idx);
}

void move_cap(cidx_t src, cap_t cap, cidx_t dst)
{
	if (src.pid != dst.pid) {
		if (cap.type == CAPTY_TIME) {
			schedule_update(dst.pid, cap.time.end, cap.time.hart,
					cap.time.mark, cap.time.end);
		} else if (cap.type == CAPTY_PMP && cap.pmp.used) {
			proc_pmp_unload(proc_get(src.pid), cap.pmp.index);
			cap.pmp.used = 0;
			cap.pmp.index = 0;
		}
	}
	ctable_move(src, cap, dst);
}

void try_revoke_non_slice_cap(cidx_t parent, cap_t parent_cap, cidx_t child,
			      cap_t child_cap)
{
	switch (child_cap.type) {
	case CAPTY_PMP:
		if (child_cap.pmp.used)
			proc_pmp_unload(proc_get(child.pid),
					child_cap.pmp.index);
	case CAPTY_SERVER:
	case CAPTY_CLIENT:
		return;
	default:
		kassert(0);
	}
}

void try_revoke_slice_cap(cidx_t parent, cap_t parent_cap, cidx_t child,
			  cap_t child_cap)
{
	switch (parent_cap.type) {
	case CAPTY_TIME:
		schedule_update(parent.pid, parent_cap.time.end,
				parent_cap.time.hart, parent_cap.time.mark,
				child_cap.time.mark);
		parent_cap.time.mark = child_cap.time.mark;
		break;
	case CAPTY_MEMORY:
		parent_cap.memory.mark = child_cap.memory.mark;
		parent_cap.memory.lock = child_cap.memory.lock;
		break;
	case CAPTY_MONITOR:
		parent_cap.monitor.mark = child_cap.monitor.mark;
		break;
	case CAPTY_CHANNEL:
		parent_cap.channel.mark = child_cap.channel.mark;
		break;
	default:
		kassert(0); // or handle unexpected type if needed
	}
	ctable_update(parent, parent_cap);
}

void try_revoke_cap(cidx_t parent, cap_t parent_cap, cidx_t child,
		    cap_t child_cap)
{
	if (!ctable_conditional_delete(child, child_cap, parent))
		return;

	if (parent_cap.type != child_cap.type)
		try_revoke_non_slice_cap(parent, parent_cap, child, child_cap);
	else
		try_revoke_slice_cap(parent, parent_cap, child, child_cap);
}

void restore_cap(cidx_t idx, cap_t cap)
{
	switch (cap.type) {
	case CAPTY_TIME:
		schedule_update(idx.pid, cap.time.end, cap.time.hart,
				cap.time.begin, cap.time.mark);
		cap.time.mark = cap.time.begin;
		break;
	case CAPTY_MEMORY:
		cap.memory.mark = cap.memory.begin;
		cap.memory.lock = 0;
		break;
	case CAPTY_MONITOR:
		cap.monitor.mark = cap.monitor.begin;
		break;
	case CAPTY_CHANNEL:
		cap.channel.mark = cap.channel.begin;
		break;
	default:
		return; // or handle unexpected type if needed
	}
	ctable_update(idx, cap);
}

static void derive_non_slice_cap(cidx_t src, cap_t src_cap, cap_t new_cap)
{
	switch (new_cap.type) {
	case CAPTY_PMP:
		src_cap.memory.lock = 1;
		break;
	case CAPTY_SERVER:
		src_cap.channel.mark = new_cap.server.channel + 1;
		break;
	case CAPTY_CLIENT:
		return;
	default:
		kassert(0);
	}
	ctable_update(src, src_cap);
}

static void derive_slice_cap(cidx_t src, cap_t src_cap, cap_t new_cap)
{
	switch (src_cap.type) {
	case CAPTY_TIME:
		schedule_update(src.pid, new_cap.time.end, new_cap.time.hart,
				new_cap.time.begin, new_cap.time.end);
		src_cap.time.mark = new_cap.time.mark;
		break;
	case CAPTY_MEMORY:
		src_cap.memory.mark = new_cap.memory.mark;
		break;
	case CAPTY_MONITOR:
		src_cap.monitor.mark = new_cap.monitor.mark;
		break;
	case CAPTY_CHANNEL:
		src_cap.channel.mark = new_cap.channel.mark;
		break;
	default:
		kassert(0);
	}

	ctable_update(src, src_cap);
}

void derive_cap(cidx_t src, cap_t src_cap, cidx_t dst, cap_t new_cap)
{
	ctable_insert(dst, new_cap, src);

	if (src_cap.type != new_cap.type)
		derive_non_slice_cap(src, src_cap, new_cap);
	else
		derive_slice_cap(src, src_cap, new_cap);
}

void pmp_load(cidx_t idx, cap_t cap, uint64_t pmpidx)
{
	proc_pmp_load(proc_get(idx.pid), pmpidx, cap.pmp.addr, cap.pmp.rwx);
	cap.pmp.used = 1;
	cap.pmp.index = (uint8_t)pmpidx;
	ctable_update(idx, cap);
}

void pmp_unload(cidx_t idx, cap_t cap)
{
	proc_pmp_unload(proc_get(idx.pid), cap.pmp.index);
	cap.pmp.used = 0;
	cap.pmp.index = 0;
	ctable_update(idx, cap);
}

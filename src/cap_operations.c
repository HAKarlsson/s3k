#include "cap_operations.h"

#include "cap_table.h"
#include "cap_types.h"
#include "cap_utils.h"
#include "ipc.h"
#include "kassert.h"
#include "preemption.h"
#include "proc.h"
#include "schedule.h"

static ticket_lock_t _lock;

static void cap_lock(void);
static void cap_unlock(void);
static cap_t cap_move_hook(uint64_t srcPid, uint64_t destPid, cap_t cap);
static void cap_delete_hook(uint64_t pid, cap_t cap);
static cap_t cap_revoke_hook(uint64_t pid, cap_t cap, uint64_t revokedPid, cap_t revokedCap);
static cap_t cap_restore_hook(uint64_t pid, cap_t cap);
static cap_t cap_derive_hook(uint64_t pid, cap_t orig_cap, cap_t new_cap);

excpt_t cap_get_cap(cptr_t cptr, uint64_t ret[1])
{
	// Assert that the capability pointer is valid.
	kassert(cptr_is_valid(cptr));

	// Retreve the capability to read.
	cap_t cap = ctable_get_cap(cptr);

	// Check if the capability slot is empty.
	if (ctable_is_null(cptr))
		return EXCPT_EMPTY;

	// Store the raw capability to return array.
	ret[0] = cap.raw;

	return EXCPT_NONE;
}

excpt_t cap_move(cptr_t src, cptr_t dest)
{
	// Assert that the capability pointers are valid.
	kassert(cptr_is_valid(src));
	kassert(cptr_is_valid(dest));

	// Retrieve the capability to move.
	cap_t cap = ctable_get_cap(src);

	// If the capability is empty.
	if (ctable_is_null(src))
		return EXCPT_EMPTY;

	// Check if the destination capbility is not empty.
	if (!ctable_is_null(dest))
		return EXCPT_COLLISION;

	// Acquire the capability lock.
	cap_lock();

	// Move the capability from src to dest.
	if (!ctable_is_null(src)) {
		ctable_move(src, dest, cap);
	}

	// Release the capability lock.
	cap_unlock();

	return EXCPT_NONE;
}

excpt_t cap_delete(cptr_t cptr)
{
	// Assert that the capability pointer is valid.
	kassert(cptr_is_valid(cptr));

	// Retrieve the process ID and capability associated with the capability pointer.
	uint64_t pid = cptr_get_pid(cptr);
	cap_t cap = ctable_get_cap(cptr);

	// If the capability is empty.
	if (ctable_is_null(cptr))
		return EXCPT_EMPTY;

	// Acquire the capability lock.
	cap_lock();

	// Delete the capability from the capability table and perform cleanup.
	if (!ctable_is_null(cptr)) {
		ctable_delete(cptr);
		cap_delete_hook(pid, cap);
	}

	// Release the capability lock.
	cap_unlock();

	return EXCPT_NONE;
}

excpt_t cap_revoke(cptr_t cptr)
{
	// Assert that the capability pointer is valid.
	kassert(cptr_is_valid(cptr));

	// Retrieve the process ID and capability associated with the capability pointer.
	uint64_t pid = cptr_get_pid(cptr);
	cap_t cap = ctable_get_cap(cptr);

	// Check if the capability is empty (null).
	if (ctable_is_null(cptr))
		return EXCPT_EMPTY;

	// Iterate through the capability table to check for children and handle revocation.
	while (true) {
		// Get the next capability pointer and its associated capability.
		cptr_t next_cptr = ctable_get_next(cptr);
		cap_t next_cap = ctable_get_cap(next_cptr);
		uint64_t next_pid = cptr_get_pid(next_cptr);

		// If our capability was deleted, return.
		if (ctable_is_null(cptr))
			return EXCPT_EMPTY;

		// If the current capability cannot be revoked with cap, break the loop.
		if (!cap_can_revoke(cap, next_cap))
			break;

		if (preemption())
			return EXCPT_PREEMPT;

		// Acquire the capability lock and perform revocation.
		cap_lock();
		if (!ctable_is_null(cptr)
		    && ctable_get_next(cptr) == next_cptr) {
			// Delete the next capability and update the current capability.
			ctable_delete(next_cptr);
			next_cap = ctable_get_cap(next_cptr);
			cap = cap_revoke_hook(pid, cap, next_pid, next_cap);
			ctable_update(cptr, cap);
		}
		// Release the capability lock.
		cap_unlock();
	}

	// Acquire the capability lock and perform final restoration.
	cap_lock();
	if (!ctable_is_null(cptr)) {
		cap = cap_restore_hook(pid, cap);
		ctable_update(cptr, cap);
	}

	// Release the capability lock.
	cap_unlock();

	return EXCPT_NONE;
}

excpt_t cap_derive(cptr_t orig, cptr_t dest, cap_t new_cap)
{
	// Assert that the capability pointers are valid.
	kassert(cptr_is_valid(orig));
	kassert(cptr_is_valid(dest));

	// Retrieve the process ID and original capability associated with the original capability pointer.
	uint64_t pid = cptr_get_pid(orig);
	cap_t orig_cap = ctable_get_cap(orig);
	uint32_t orig_depth = ctable_get_depth(orig);

	// Check if the original capability pointer is empty (null).
	if (ctable_is_null(orig))
		return EXCPT_EMPTY;

	// Check if the destination capability pointer is already occupied.
	if (!ctable_is_null(dest))
		return EXCPT_COLLISION;

	// Check if the new capability can be derived from the original capability.
	if (!cap_can_derive(orig_cap, new_cap)
	    || orig_depth == CTABLE_MAX_DEPTH)
		return EXCPT_DERIVATION;

	// Acquire the capability lock and perform the derivation.
	cap_lock();
	if (!ctable_is_null(orig)) {
		ctable_insert(orig, dest, new_cap);
		orig_cap = cap_derive_hook(pid, orig_cap, new_cap);
		ctable_update(orig, orig_cap);
	}
	// Release the capability lock.
	cap_unlock();

	return EXCPT_NONE;
}

excpt_t cap_pmp_set(cptr_t cptr, uint64_t index)
{
	kassert(cptr_is_valid(cptr));

	uint64_t pid = cptr_get_pid(cptr);
	proc_t *proc = proc_get(pid);
	cap_t cap = ctable_get_cap(cptr);

	if (ctable_is_null(cptr))
		return EXCPT_EMPTY;
	if (cap.type != CAPTY_PMP)
		return EXCPT_INVALID_CAP;
	if (proc_pmp_is_set(proc, index) || cap.pmp.used)
		return EXCPT_COLLISION;

	cap_lock();
	if (!ctable_is_null(cptr)) {
		proc_pmp_set(proc, index, cap.pmp.addr, cap.pmp.rwx);
		cap.pmp.index = index;
		cap.pmp.used = 1;
		ctable_update(cptr, cap);
	}
	cap_unlock();
	return EXCPT_NONE;
}

excpt_t cap_pmp_clear(cptr_t cptr)
{
	kassert(cptr_is_valid(cptr));

	uint64_t pid = cptr_get_pid(cptr);
	proc_t *proc = proc_get(pid);
	cap_t cap = ctable_get_cap(cptr);

	if (ctable_is_null(cptr))
		return EXCPT_EMPTY;
	if (cap.type != CAPTY_PMP)
		return EXCPT_INVALID_CAP;
	if (!cap.pmp.used)
		return EXCPT_NONE;

	cap_lock();
	if (!ctable_is_null(cptr)) {
		proc_pmp_clear(proc, cap.pmp.index);
		cap.pmp.index = 0;
		cap.pmp.used = 0;
		ctable_update(cptr, cap);
	}
	cap_unlock();
	return EXCPT_NONE;
}

bool monitor_validate(cptr_t cptr, uint64_t pid)
{
	cap_t cap = ctable_get_cap(cptr);
	if (ctable_is_null(cptr))
		return EXCPT_EMPTY;
	if (cap.type != CAPTY_MONITOR)
		return EXCPT_INVALID_CAP;
	if (pid < cap.monitor.free || pid >= cap.monitor.end)
		return EXCPT_MONITOR_PID;
	return EXCPT_NONE;
}

excpt_t cap_monitor_suspend(cptr_t mon_cptr, uint64_t pid)
{
	kassert(cptr_is_valid(mon_cptr));

	excpt_t error = monitor_validate(mon_cptr, pid);
	if (error)
		return error;

	proc_t *proc = proc_get(pid);

	cap_lock();
	if (ctable_is_null(mon_cptr)) {
		error = EXCPT_EMPTY;
	} else {
		proc_suspend(proc);
	}
	cap_unlock();
	return error;
}

excpt_t cap_monitor_resume(cptr_t mon_cptr, uint64_t pid)
{
	kassert(cptr_is_valid(mon_cptr));

	excpt_t error = monitor_validate(mon_cptr, pid);
	if (error)
		return error;

	proc_t *proc = proc_get(pid);

	cap_lock();
	if (ctable_is_null(mon_cptr)) {
		error = EXCPT_EMPTY;
	} else {
		proc_resume(proc);
	}
	cap_unlock();
	return error;
}

excpt_t cap_monitor_get_reg(cptr_t mon_cptr, uint64_t pid, uint64_t reg, uint64_t ret[1])
{
	kassert(cptr_is_valid(mon_cptr));
	kassert(reg < NUM_OF_REGS);

	excpt_t error = monitor_validate(mon_cptr, pid);
	if (error)
		return error;
	proc_t *proc = proc_get(pid);

	cap_lock();
	if (ctable_is_null(mon_cptr)) {
		error = EXCPT_EMPTY;
	} else if (!proc_monitor_acquire(proc)) {
		error = EXCPT_MONITOR_BUSY;
	} else {
		ret[0] = proc->regs[reg];
		proc_release(proc);
	}
	cap_unlock();
	return error;
}

excpt_t cap_monitor_set_reg(cptr_t mon_cptr, uint64_t pid, uint64_t reg, uint64_t val)
{
	kassert(cptr_is_valid(mon_cptr));
	kassert(reg < NUM_OF_REGS);

	excpt_t error = monitor_validate(mon_cptr, pid);
	if (error)
		return error;
	proc_t *proc = proc_get(pid);

	cap_lock();
	if (ctable_is_null(mon_cptr)) {
		error = EXCPT_EMPTY;
	} else if (!proc_monitor_acquire(proc)) {
		error = EXCPT_MONITOR_BUSY;
	} else {
		proc->regs[reg] = val;
		proc_release(proc);
	}
	cap_unlock();
	return error;
}

excpt_t cap_monitor_get_cap(cptr_t mon_cptr, uint64_t pid, uint64_t cidx, uint64_t ret[1])
{
	kassert(cptr_is_valid(mon_cptr));

	excpt_t error = monitor_validate(mon_cptr, pid);
	if (error)
		return error;
	proc_t *proc = proc_get(pid);

	cap_lock();
	if (ctable_is_null(mon_cptr)) {
		error = EXCPT_EMPTY;
	} else if (!proc_monitor_acquire(proc)) {
		error = EXCPT_MONITOR_BUSY;
	} else {
		cptr_t cptr = cptr_mk(pid, cidx);
		cap_t cap = ctable_get_cap(cptr);
		ret[0] = cap.raw;
		proc_release(proc);
	}
	cap_unlock();
	return error;
}

excpt_t cap_monitor_move_cap(cptr_t mon_cptr, uint64_t pid, uint64_t src_cidx, uint64_t dest_cidx, bool take)
{
	kassert(cptr_is_valid(mon_cptr));

	excpt_t error = monitor_validate(mon_cptr, pid);
	if (error)
		return error;

	proc_t *proc = proc_get(pid);

	uint64_t src_pid = take ? pid : cptr_get_pid(mon_cptr);
	uint64_t dest_pid = take ? cptr_get_pid(mon_cptr) : pid;
	cptr_t src = cptr_mk(src_pid, src_cidx);
	cptr_t dest = cptr_mk(dest_pid, dest_cidx);

	cap_lock();
	if (ctable_is_null(mon_cptr)) {
		error = EXCPT_EMPTY;
	} else if (ctable_is_null(src)) {
		error = EXCPT_MONITOR_EMPTY;
	} else if (!ctable_is_null(dest)) {
		error = EXCPT_MONITOR_COLLISION;
	} else if (!proc_monitor_acquire(proc)) {
		error = EXCPT_MONITOR_BUSY;
	} else {
		cap_t cap = ctable_get_cap(src);
		ctable_move(src, dest, cap);
		cap_move_hook(src_pid, dest_pid, cap);
		proc_release(proc);
	}
	cap_unlock();
	return error;
}

void cap_lock(void)
{
	tl_acq(&_lock);
}

void cap_unlock(void)
{
	tl_rel(&_lock);
}

cap_t cap_move_hook(uint64_t src_pid, uint64_t dest_pid, cap_t cap)
{
	if (src_pid == dest_pid)
		return cap;

	switch (cap.type) {
	case CAPTY_TIME: {
		uint64_t end = cap.time.end;
		uint64_t hartid = cap.time.hartid;
		uint64_t from = cap.time.free;
		uint64_t to = cap.time.end;
		schedule_update(dest_pid, end, hartid, from, to);
	} break;

	case CAPTY_PMP: {
		if (!cap.pmp.used)
			break;
		proc_pmp_clear(proc_get(src_pid), cap.pmp.index);
		cap.pmp.used = 0;
		cap.pmp.index = 0;
	} break;
	}

	return cap;
}

void cap_delete_hook(uint64_t pid, cap_t cap)
{
	switch (cap.type) {
	case CAPTY_TIME: {
		schedule_delete(cap.time.hartid, cap.time.free, cap.time.end);
	} break;

	case CAPTY_PMP: {
		proc_pmp_clear(proc_get(pid), cap.pmp.index);
	} break;
	}
}

cap_t cap_revoke_hook(uint64_t pid, cap_t cap, uint64_t rev_pid, cap_t rev_cap)
{
	switch (rev_cap.type) {
	case CAPTY_TIME: {
		schedule_update(pid, cap.time.end,
				cap.time.hartid, rev_cap.time.begin, cap.time.free);
		cap.time.free = rev_cap.time.begin;
	} break;

	case CAPTY_MEMORY: {
		cap.memory.free = rev_cap.memory.begin;
		cap.memory.lock = rev_cap.memory.lock;
	} break;

	case CAPTY_PMP: {
		if (!rev_cap.pmp.used)
			break;
		proc_pmp_clear(proc_get(pid), rev_cap.pmp.index);
	} break;

	case CAPTY_MONITOR: {
		cap.monitor.free = rev_cap.monitor.begin;
	} break;

	case CAPTY_CHANNEL: {
		cap.channel.free = rev_cap.channel.begin;
	} break;
	}
	return cap;
}

cap_t cap_restore_hook(uint64_t pid, cap_t cap)
{
	switch (cap.type) {
	case CAPTY_TIME: {
		schedule_update(pid, cap.time.end,
				cap.time.hartid, cap.time.begin, cap.time.free);
		cap.time.free = cap.time.begin;
	} break;

	case CAPTY_MEMORY: {
		cap.memory.free = cap.memory.begin;
	} break;

	case CAPTY_MONITOR: {
		cap.monitor.free = cap.monitor.begin;
	} break;

	case CAPTY_CHANNEL: {
		cap.channel.free = cap.channel.begin;
	} break;
	}
	return cap;
}

cap_t cap_derive_hook(uint64_t pid, cap_t orig_cap, cap_t new_cap)
{
	switch (new_cap.type) {
	case CAPTY_TIME: {
		schedule_update(pid, new_cap.time.end,
				new_cap.time.hartid, new_cap.time.begin, new_cap.time.end);
		orig_cap.time.free = new_cap.time.end;
	} break;
	case CAPTY_MEMORY: {
		orig_cap.memory.free = new_cap.memory.end;
	} break;
	case CAPTY_PMP: {
		orig_cap.memory.lock = 1;
	} break;
	case CAPTY_MONITOR: {
		orig_cap.monitor.free = new_cap.monitor.end;
	} break;
	case CAPTY_CHANNEL: {
		orig_cap.channel.free = new_cap.channel.end;
	} break;
	case CAPTY_SOCKET: {
		if (new_cap.socket.tag == 0)
			orig_cap.channel.free = new_cap.socket.channel + 1;
	} break;
	}
	return orig_cap;
}

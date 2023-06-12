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

int cap_get_cap(cptr_t cptr, uint64_t ret[1])
{
	// Assert that the capability pointer is valid.
	kassert(cptr_is_valid(cptr));

	// Retreve the capability to read.
	cap_t cap = ctable_get_cap(cptr);

	// Store the raw capability to return array.
	ret[0] = cap.raw;
	return EXCPT_NONE;
}

int cap_move(cptr_t src, cptr_t dest)
{
	// Assert that the capability pointers are valid.
	kassert(cptr_is_valid(src));
	kassert(cptr_is_valid(dest));

	// Retrieve the capability to move.
	cap_t cap = ctable_get_cap(src);

	// If the capability is empty.
	if (ctable_is_null(src))
		return EXCPT_EMPTY;

	// If the destination capbility is not empty.
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

int cap_delete(cptr_t cptr)
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

int cap_revoke(cptr_t cptr)
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

	if (preemption())
		return EXCPT_PREEMPT;

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

int cap_derive(cptr_t orig, cptr_t dest, cap_t new_cap)
{
	// Assert that the capability pointers are valid.
	kassert(cptr_is_valid(orig));
	kassert(cptr_is_valid(dest));

	// Retrieve the process ID and original capability associated with the original capability pointer.
	uint64_t pid = cptr_get_pid(orig);
	cap_t orig_cap = ctable_get_cap(orig);

	// Check if the original capability pointer is empty (null).
	if (ctable_is_null(orig))
		return EXCPT_EMPTY;

	// Check if the destination capability pointer is already occupied.
	if (!ctable_is_null(dest))
		return EXCPT_COLLISION;

	if (preemption())
		return EXCPT_PREEMPT;

	// Check if the new capability can be derived from the original capability.
	if (!cap_can_derive(orig_cap, new_cap))
		return EXCPT_DERIVATION;

	if (preemption())
		return EXCPT_PREEMPT;

	// Acquire the capability lock and perform the derivation.
	cap_lock();
	// If cptr point to null cap, it was revoked.
	if (!ctable_is_null(orig)) {
		ctable_insert(orig, dest, new_cap);
		orig_cap = cap_derive_hook(pid, orig_cap, new_cap);
		ctable_update(orig, orig_cap);
	}
	// Release the capability lock.
	cap_unlock();

	return EXCPT_NONE;
}

int cap_pmp_set(cptr_t cptr, uint64_t index)
{
	// Assert that the capability pointer is valid.
	kassert(cptr_is_valid(cptr));

	// Retrieve the capability
	cap_t cap = ctable_get_cap(cptr);
	// Get the pid of the capability
	uint64_t pid = cptr_get_pid(cptr);
	// Get the corresponding process
	proc_t *proc = proc_get(pid);

	// Check if capability is null.
	if (ctable_is_null(cptr))
		return EXCPT_EMPTY;

	// Check if capability has the right type.
	if (cap.type != CAPTY_PMP)
		return EXCPT_INVALID_CAP;

	// Check if we can set the pmp.
	// If pmp cap is used, or index is ued, then collision.
	if (proc_pmp_is_set(proc, index) || cap.pmp.used)
		return EXCPT_COLLISION;

	// Acquire the capability lock.
	cap_lock();
	// If cptr point to null cap, then it was revoked.
	if (!ctable_is_null(cptr)) {
		// If cap still exists, perform the pmp set.
		proc_pmp_set(proc, index, cap.pmp.addr, cap.pmp.rwx);
		proc_pmp_load(proc);
		cap.pmp.index = index;
		cap.pmp.used = 1;
		ctable_update(cptr, cap);
	}
	// Release the capability lock.
	cap_unlock();
	return EXCPT_NONE;
}

int cap_pmp_clear(cptr_t cptr)
{
	kassert(cptr_is_valid(cptr));

	cap_t cap = ctable_get_cap(cptr);
	uint64_t pid = cptr_get_pid(cptr);
	proc_t *proc = proc_get(pid);

	if (ctable_is_null(cptr))
		return EXCPT_EMPTY;

	// Check if capability has the right type.
	if (cap.type != CAPTY_PMP)
		return EXCPT_INVALID_CAP;

	// If already clear, nothing happens.
	if (!cap.pmp.used)
		return EXCPT_NONE;

	// Acquire the capability lock.
	cap_lock();
	// If cptr point to null cap, then it was revoked.
	if (!ctable_is_null(cptr)) {
		// If cap still exists, perform the pmp clear.
		proc_pmp_clear(proc, cap.pmp.index);
		proc_pmp_load(proc);
		cap.pmp.index = 0;
		cap.pmp.used = 0;
		ctable_update(cptr, cap);
	}
	// Release the capability lock.
	cap_unlock();
	return EXCPT_NONE;
}

int monitor_validate(cptr_t mon_cptr, uint64_t pid)
{
	cap_t cap = ctable_get_cap(mon_cptr);
	if (cap.type == CAPTY_NULL)
		return EXCPT_EMPTY;
	if (cap.type != CAPTY_MONITOR)
		return EXCPT_INVALID_CAP;
	uint64_t free = cap.monitor.base + cap.monitor.alloc;
	uint64_t end = cap.monitor.base + cap.monitor.len;
	if (pid < free || pid >= end)
		return EXCPT_MONITOR_PID;
	return EXCPT_NONE;
}

int cap_monitor_suspend(cptr_t mon_cptr, uint64_t pid)
{
	kassert(cptr_is_valid(mon_cptr));

	int error = monitor_validate(mon_cptr, pid);
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

int cap_monitor_resume(cptr_t mon_cptr, uint64_t pid)
{
	kassert(cptr_is_valid(mon_cptr));

	int error = monitor_validate(mon_cptr, pid);
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

int cap_monitor_get_reg(cptr_t mon_cptr, uint64_t pid, uint64_t reg, uint64_t ret[1])
{
	kassert(cptr_is_valid(mon_cptr));
	kassert(reg < NUM_OF_REGS);

	int error = monitor_validate(mon_cptr, pid);
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

int cap_monitor_set_reg(cptr_t mon_cptr, uint64_t pid, uint64_t reg, uint64_t val)
{
	kassert(cptr_is_valid(mon_cptr));
	kassert(reg < NUM_OF_REGS);

	int error = monitor_validate(mon_cptr, pid);
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

int cap_monitor_get_cap(cptr_t mon_cptr, uint64_t pid, cptr_t cptr, uint64_t ret[1])
{
	kassert(cptr_is_valid(mon_cptr));
	kassert(cptr_is_valid(cptr));

	int error = monitor_validate(mon_cptr, pid);
	if (error)
		return error;
	proc_t *proc = proc_get(pid);

	cap_lock();
	if (ctable_is_null(mon_cptr)) {
		error = EXCPT_EMPTY;
	} else if (!proc_monitor_acquire(proc)) {
		error = EXCPT_MONITOR_BUSY;
	} else {
		cap_t cap = ctable_get_cap(cptr);
		ret[0] = cap.raw;
		proc_release(proc);
	}
	cap_unlock();
	return error;
}

int cap_monitor_move_cap(cptr_t mon_cptr, uint64_t pid, cptr_t src_cptr, cptr_t dest_cptr)
{
	kassert(cptr_is_valid(mon_cptr));
	kassert(cptr_is_valid(src_cptr));
	kassert(cptr_is_valid(dest_cptr));

	int error = monitor_validate(mon_cptr, pid);
	if (error)
		return error;
	proc_t *proc = proc_get(pid);

	uint64_t src_pid = cptr_get_pid(src_cptr);
	uint64_t dest_pid = cptr_get_pid(dest_cptr);

	cap_lock();
	if (ctable_is_null(mon_cptr)) {
		error = EXCPT_EMPTY;
	} else if (ctable_is_null(src_cptr)) {
		error = EXCPT_MONITOR_EMPTY;
	} else if (!ctable_is_null(dest_cptr)) {
		error = EXCPT_MONITOR_COLLISION;
	} else if (!proc_monitor_acquire(proc)) {
		error = EXCPT_MONITOR_BUSY;
	} else {
		cap_t cap = ctable_get_cap(src_cptr);
		ctable_move(src_cptr, dest_cptr, cap);
		cap_move_hook(src_pid, dest_pid, cap);
		proc_release(proc);
	}
	cap_unlock();
	return error;
}

int cap_monitor_pmp_set(cptr_t mon_cptr, uint64_t pid, cptr_t pmp_cptr, uint64_t index)
{
	kassert(cptr_is_valid(mon_cptr));
	kassert(cptr_is_valid(pmp_cptr));

	int error = monitor_validate(mon_cptr, pid);
	if (error)
		return error;
	proc_t *proc = proc_get(pid);

	cap_lock();
	cap_t pmp_cap = ctable_get_cap(pmp_cptr);
	if (ctable_is_null(mon_cptr)) {
		error = EXCPT_EMPTY;
	} else if (ctable_is_null(pmp_cptr)) {
		error = EXCPT_MONITOR_EMPTY;
	} else if (pmp_cap.type != CAPTY_PMP) {
		error = EXCPT_INVALID_CAP;
	} else if (pmp_cap.pmp.used || proc_pmp_is_set(proc, index)) {
		error = EXCPT_COLLISION;
	} else if (!proc_monitor_acquire(proc)) {
		error = EXCPT_MONITOR_BUSY;
	} else {
		proc_pmp_set(proc, index, pmp_cap.pmp.addr, pmp_cap.pmp.rwx);
		pmp_cap.pmp.used = 1;
		pmp_cap.pmp.index = index;
		ctable_update(pmp_cptr, pmp_cap);
		proc_release(proc);
	}
	cap_unlock();
	return error;
}

int cap_monitor_pmp_clear(cptr_t mon_cptr, uint64_t pid, cptr_t pmp_cptr)
{
	kassert(cptr_is_valid(mon_cptr));
	kassert(cptr_is_valid(pmp_cptr));

	int error = monitor_validate(mon_cptr, pid);
	if (error)
		return error;
	proc_t *proc = proc_get(pid);

	cap_lock();
	cap_t pmp_cap = ctable_get_cap(pmp_cptr);
	if (ctable_is_null(mon_cptr)) {
		error = EXCPT_EMPTY;
	} else if (ctable_is_null(pmp_cptr)) {
		error = EXCPT_MONITOR_EMPTY;
	} else if (pmp_cap.type != CAPTY_PMP) {
		error = EXCPT_INVALID_CAP;
	} else if (!pmp_cap.pmp.used) {
		error = EXCPT_NONE;
	} else if (!proc_monitor_acquire(proc)) {
		error = EXCPT_MONITOR_BUSY;
	} else {
		proc_pmp_clear(proc, pmp_cap.pmp.index);
		pmp_cap.pmp.used = 0;
		pmp_cap.pmp.index = 0;
		ctable_update(pmp_cptr, pmp_cap);
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
		uint64_t end = cap.time.base + cap.time.len;
		;
		uint64_t hartid = cap.time.hartid;
		uint64_t from = cap.time.base + cap.time.alloc;
		;
		uint64_t to = cap.time.base + cap.time.len;
		;
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
		uint64_t hartid = cap.time.hartid;
		uint64_t from = cap.time.base + cap.time.alloc;
		uint64_t to = cap.time.base + cap.time.len;
		schedule_delete(hartid, from, to);
	} break;

	case CAPTY_PMP: {
		if (cap.pmp.used)
			proc_pmp_clear(proc_get(pid), cap.pmp.index);
	} break;
	}
}

cap_t cap_revoke_hook(uint64_t pid, cap_t cap, uint64_t rev_pid, cap_t rev_cap)
{
	switch (rev_cap.type) {
	case CAPTY_TIME: {
		uint64_t end = cap.time.base + cap.time.len;
		uint64_t hartid = cap.time.hartid;
		uint64_t from = cap.time.base + cap.time.alloc;
		uint64_t to = cap.time.base + cap.time.len;
		schedule_update(pid, end, hartid, from, to);
		cap.time.alloc = rev_cap.time.base - cap.time.base;
	} break;

	case CAPTY_MEMORY: {
		cap.memory.alloc = rev_cap.memory.base - cap.memory.base;
		cap.memory.lock = rev_cap.memory.lock;
	} break;

	case CAPTY_PMP: {
		if (!rev_cap.pmp.used)
			break;
		proc_pmp_clear(proc_get(pid), rev_cap.pmp.index);
	} break;

	case CAPTY_MONITOR: {
		cap.monitor.alloc = rev_cap.monitor.base - cap.monitor.base;
	} break;

	case CAPTY_CHANNEL: {
		cap.channel.alloc = rev_cap.channel.base - cap.channel.base;
	} break;
	}
	return cap;
}

cap_t cap_restore_hook(uint64_t pid, cap_t cap)
{
	switch (cap.type) {
	case CAPTY_TIME: {
		uint64_t end = cap.time.base + cap.time.len;
		uint64_t hartid = cap.time.hartid;
		uint64_t from = cap.time.base;
		uint64_t to = cap.time.base + cap.time.alloc;
		schedule_update(pid, end,
				hartid, from, to);
		cap.time.alloc = 0;
	} break;

	case CAPTY_MEMORY: {
		cap.memory.alloc = 0;
	} break;

	case CAPTY_MONITOR: {
		cap.monitor.alloc = 0;
	} break;

	case CAPTY_CHANNEL: {
		cap.channel.alloc = 0;
	} break;
	}
	return cap;
}

cap_t cap_derive_hook(uint64_t pid, cap_t orig_cap, cap_t new_cap)
{
	switch (new_cap.type) {
	case CAPTY_TIME: {
		uint64_t end = new_cap.time.base + new_cap.time.len;
		uint64_t hartid = new_cap.time.hartid;
		uint64_t from = new_cap.time.base;
		uint64_t to = new_cap.time.base + new_cap.time.len;
		schedule_update(pid, end, hartid, from, to);
		orig_cap.time.alloc += new_cap.time.len;
	} break;
	case CAPTY_MEMORY: {
		orig_cap.memory.alloc += new_cap.memory.len;
	} break;
	case CAPTY_PMP: {
		orig_cap.memory.lock = 1;
	} break;
	case CAPTY_MONITOR: {
		orig_cap.monitor.alloc += new_cap.monitor.len;
	} break;
	case CAPTY_CHANNEL: {
		orig_cap.channel.alloc += new_cap.channel.len;
	} break;
	case CAPTY_SOCKET: {
		if (new_cap.socket.tag == 0)
			orig_cap.channel.alloc++;
	} break;
	}
	return orig_cap;
}

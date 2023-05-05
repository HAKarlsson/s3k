/* See LICENSE file for copyright and license details. */
#include "syscall.h"

#include "cap.h"
#include "cnode.h"
#include "common.h"
#include "consts.h"
#include "csr.h"
#include "excpt.h"
#include "kassert.h"
#include "preemption.h"
#include "proc.h"
#include "schedule.h"
#include "ticket_lock.h"
#include "timer.h"

// static proc_t *_listeners[NCHANNEL];

register proc_t *current __asm__("tp");
static ticket_lock_t _syscall_lock;

static void syscall_lock(void)
{
	tl_acq(&_syscall_lock);
}

static void syscall_unlock(void)
{
	tl_rel(&_syscall_lock);
}

static uint64_t syscall_get_info(uint64_t info);
static uint64_t syscall_get_reg(uint64_t reg);
static void syscall_set_reg(uint64_t reg, uint64_t value);
static void syscall_yield(void);
static uint64_t syscall_read_cap(uint64_t cidx);
static uint64_t syscall_move_cap(uint64_t src_cidx, uint64_t dest_cidx);
static uint64_t syscall_delete_cap(uint64_t cidx);
static uint64_t syscall_revoke_cap(uint64_t cidx);
static uint64_t syscall_derive_cap(uint64_t parent_cidx, uint64_t child_cidx,
				   uint64_t cap_raw);
static uint64_t syscall_invoke_cap(uint64_t sysnr, uint64_t cidx);
static uint64_t syscall_invoke_pmp(uint64_t sysnr, cnode_handle_t node,
				   cap_t cap);
static uint64_t syscall_invoke_monitor(uint64_t sysnr, cnode_handle_t node,
				       cap_t cap);
static uint64_t syscall_invoke_socket(uint64_t sysnr, cnode_handle_t node,
				      cap_t cap);

void syscall_handler(uint64_t sysnr, uint64_t a1, uint64_t a2, uint64_t a3,
		     uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7)
{
	// increment PC
	current->regs.pc += 4;
	// Find appropriate system call
	switch (sysnr) {
	case SYSCALL_GET_INFO:
		current->regs.a0 = syscall_get_info(a1);
		break;
	case SYSCALL_GET_REG:
		current->regs.a0 = syscall_get_reg(a1);
		break;
	case SYSCALL_SET_REG:
		syscall_set_reg(a1, a2);
		break;
	case SYSCALL_YIELD:
		syscall_yield();
		break;
	case SYSCALL_READ_CAP:
		current->regs.a0 = syscall_read_cap(a1);
		break;
	case SYSCALL_MOVE_CAP:
		current->regs.a0 = syscall_move_cap(a1, a2);
		break;
	case SYSCALL_DELETE_CAP:
		current->regs.a0 = syscall_delete_cap(a1);
		break;
	case SYSCALL_REVOKE_CAP:
		current->regs.a0 = syscall_revoke_cap(a1);
		break;
	case SYSCALL_DERIVE_CAP:
		current->regs.a0 = syscall_derive_cap(a1, a2, a3);
		break;
	default:
		current->regs.a0 = syscall_invoke_cap(sysnr, a1);
		break;
	}
}

uint64_t syscall_get_info(uint64_t nr)
{
	return 0;
}

uint64_t syscall_get_reg(uint64_t reg)
{
	uint64_t val;
	uint64_t *regs = (uint64_t *)&current->regs;
	if (reg < REG_COUNT) {
		val = regs[reg];
	} else {
		val = 0;
	}
	return val;
}

void syscall_set_reg(uint64_t reg, uint64_t value)
{
	uint64_t *regs = (uint64_t *)&current->regs;
	if (reg < REG_COUNT)
		regs[reg] = value;
}

void syscall_yield(void)
{
	schedule_yield();
}

uint64_t syscall_read_cap(uint64_t cidx)
{
	if (cidx >= CAP_COUNT)
		return EXCPT_INDEX;
	cnode_handle_t node = cnode_get(current->pid, cidx);
	cap_t cap = cnode_cap(node);
	current->regs.a1 = cap.raw;
	return EXCPT_NONE;
}

uint64_t syscall_move_cap(uint64_t src_idx, uint64_t dst_idx)
{
	if (src_idx >= CAP_COUNT || dst_idx >= CAP_COUNT)
		return EXCPT_INDEX;

	cnode_handle_t src = cnode_get(current->pid, src_idx);
	cnode_handle_t dst = cnode_get(current->pid, dst_idx);

	// Check if capability exists
	if (cnode_is_null(src))
		return EXCPT_EMPTY;

	// Check if destination is occupied
	if (!cnode_is_null(dst))
		return EXCPT_COLLISION;

	syscall_lock();
	excpt_t status = cnode_move(src, dst) ? EXCPT_NONE : EXCPT_EMPTY;
	syscall_unlock();
	return status;
}

static void syscall_delete_cleanup(cap_t cap)
{
	// If the capability is PMP, then unset the pmp config if it was used.
	if (cap.type == CAPTY_PMP && cap.pmp.used) {
		proc_pmp_clear(current, cap.pmp.idx);
		proc_pmp_load(current);
	}

	// If the capability is time, then delete the time from schedule if
	// used.
	if (cap.type == CAPTY_TIME && cap.time.free < cap.time.end)
		schedule_delete(cap.time.hartid, cap.time.free, cap.time.end);
}

uint64_t syscall_delete_cap(uint64_t cidx)
{
	if (cidx >= CAP_COUNT)
		return EXCPT_INDEX;

	cnode_handle_t node = cnode_get(current->pid, cidx);
	cap_t cap = cnode_cap(node);
	excpt_t status;

	syscall_lock();
	status = cnode_delete(node);
	if (status == EXCPT_NONE)
		syscall_delete_cleanup(cap);
	syscall_unlock();
	return status;
}

static excpt_t syscall_derive_update(cnode_handle_t parent_node,
				     cap_t parent_cap,
				     cnode_handle_t child_node, cap_t child_cap)
{
	// Set how the source cap should be updated.
	switch (child_cap.type) {
	case CAPTY_TIME:
		parent_cap.time.free = child_cap.time.begin;
		break;
	case CAPTY_MEMORY:
		parent_cap.memory.free = child_cap.memory.begin;
		break;
	case CAPTY_PMP:
		parent_cap.memory.lock = 1;
		break;
	case CAPTY_CHANNEL:
		parent_cap.channel.free = child_cap.channel.begin;
		break;
	case CAPTY_SOCKET:
		if (child_cap.socket.tag == 0) // Parent is channel
			parent_cap.channel.free = child_cap.socket.channel + 1;
		// Otherwise parent is socket
		break;
	default:
		__builtin_unreachable();
	}

	excpt_t status = cnode_update(parent_node, parent_cap);
	if (status != EXCPT_NONE)
		return status;

	if (child_cap.type == CAPTY_TIME)
		schedule_update(current->pid, child_cap.time.hartid,
				child_cap.time.free, child_cap.time.end);
	return EXCPT_NONE;
}

uint64_t syscall_derive_cap(uint64_t parent_cidx, uint64_t child_cidx,
			    uint64_t child_raw)
{
	if (parent_cidx >= CAP_COUNT || child_cidx >= CAP_COUNT)
		return EXCPT_INDEX;

	cnode_handle_t parent_node = cnode_get(current->pid, parent_cidx);
	cnode_handle_t child_node = cnode_get(current->pid, child_cidx);

	cap_t parent_cap = cnode_cap(parent_node);
	cap_t child_cap = (cap_t){ .raw = child_raw };

	if (cnode_is_null(parent_node))
		return EXCPT_EMPTY;

	if (!cnode_is_null(child_node))
		return EXCPT_COLLISION;

	if (!cap_can_derive(parent_cap, child_cap))
		return EXCPT_DERIVATION;

	if (preemption())
		return EXCPT_PREEMPTED;

	syscall_lock();
	excpt_t status = syscall_derive_update(parent_node, parent_cap,
					       child_node, child_cap);
	syscall_unlock();
	return status;
}

// Clean up resources of the child cap
static void syscall_revoke_cleanup(cnode_handle_t parent_node, cap_t parent_cap,
				   cnode_handle_t child_node, cap_t child_cap)
{
	// Try delete child
	if (!cnode_delete_if(child_node, child_cap, parent_node))
		return;

	// Update parent
	switch (child_cap.type) {
	case CAPTY_TIME:
		parent_cap.time.free = child_cap.time.free;
		break;
	case CAPTY_MEMORY:
		parent_cap.memory.free = child_cap.memory.free;
		parent_cap.memory.lock = child_cap.memory.lock;
		break;
	case CAPTY_CHANNEL:
		parent_cap.channel.free = child_cap.channel.free;
		break;
	default:
	}

	if (!cnode_update(parent_node, parent_cap))
		return;

	if (child_cap.type == CAPTY_TIME
	    && child_cap.time.free < child_cap.time.end) {
		// If child was using time, give the time to the parent.
		schedule_update(current->pid, parent_cap.time.hartid,
				parent_cap.time.free, parent_cap.time.end);
	}

	if (child_cap.type == CAPTY_PMP && child_cap.pmp.used) {
		// If pmp was used, clear relevant pmp config.
		proc_t *proc = proc_get(cnode_pid(child_node));
		proc_pmp_clear(proc, child_cap.pmp.idx);
		if (proc == current)
			proc_pmp_load(current);
	}
}

// Get the restored capability.
static excpt_t syscall_revoke_restore(cnode_handle_t node, cap_t cap)
{
	switch (cap.type) {
	case CAPTY_TIME:
		cap.time.free = cap.time.begin;
		break;
	case CAPTY_MEMORY:
		cap.memory.free = cap.memory.begin;
		break;
	case CAPTY_MONITOR:
		cap.monitor.free = cap.monitor.begin;
		break;
	case CAPTY_CHANNEL:
		cap.channel.free = cap.channel.begin;
		break;
	}

	if (!cnode_update(node, cap))
		return EXCPT_EMPTY;

	if (cap.type == CAPTY_TIME)
		schedule_update(current->pid, cap.time.hartid, cap.time.free,
				cap.time.end);

	return EXCPT_NONE;
}

uint64_t syscall_revoke_cap(uint64_t cidx)
{
	if (cidx >= CAP_COUNT)
		return EXCPT_INDEX;

	cnode_handle_t parent_node = cnode_get(current->pid, cidx);
	cap_t parent_cap = cnode_cap(parent_node);

	cnode_handle_t next_node = cnode_next(parent_node);
	cap_t next_cap = cnode_cap(next_node);

	while (!cnode_is_null(parent_node)
	       && cap_is_child(parent_cap, next_cap)) {
		if (preemption())
			return EXCPT_PREEMPTED;
		syscall_lock();
		// try to delete the child and take back the resources.
		syscall_revoke_cleanup(parent_node, parent_cap, next_node,
				       next_cap);
		syscall_unlock();
	}

	if (preemption())
		return EXCPT_PREEMPTED;

	syscall_lock();
	// Restore the cap and system.
	excpt_t status = syscall_revoke_restore(parent_node, parent_cap);
	syscall_unlock();
	return status;
}

uint64_t syscall_invoke_cap(uint64_t sysnr, uint64_t cidx)
{
	if (cidx >= CAP_COUNT)
		return EXCPT_INDEX;

	cnode_handle_t node = cnode_get(current->pid, cidx);
	cap_t cap = cnode_cap(node);

	switch (cap.type) {
	case CAPTY_NONE:
		return EXCPT_EMPTY;
	case CAPTY_PMP:
		return syscall_invoke_pmp(sysnr, node, cap);
	case CAPTY_MONITOR:
		return syscall_invoke_monitor(sysnr, node, cap);
	case CAPTY_SOCKET:
		return syscall_invoke_socket(sysnr, node, cap);
	default:
		return EXCPT_UNIMPLEMENTED;
	}
}

uint64_t syscall_invoke_pmp(uint64_t sysnr, cnode_handle_t node, cap_t cap)
{
	switch (sysnr) {
	case SYSCALL_PMP_SET: {
		uint64_t index = current->regs.a2;
		if (index >= PMP_COUNT)
			return EXCPT_INDEX;
		if (cap.pmp.used || proc_pmp_is_set(current, index))
			return EXCPT_COLLISION;
		excpt_t status;
		syscall_lock();
		cap.pmp.used = 1;
		cap.pmp.idx = index;
		if (cnode_update(node, cap)) {
			status = EXCPT_NONE;
			proc_pmp_set(current, index, cap.pmp.addr, cap.pmp.rwx);
		} else {
			status = EXCPT_EMPTY;
		}
		syscall_unlock();
		return status;
	}
	case SYSCALL_PMP_CLEAR: {
		if (cap.pmp.used)
			return EXCPT_NONE;
		proc_pmp_clear(current, cap.pmp.idx);
		cap.pmp.used = 0;
		cap.pmp.idx = 0;
		excpt_t status;
		syscall_lock();
		status = cnode_update(node, cap) ? EXCPT_NONE : EXCPT_EMPTY;
		syscall_unlock();
		return status;
	}
	default:
		return EXCPT_UNIMPLEMENTED;
	}
}

uint64_t syscall_invoke_monitor(uint64_t sysnr, cnode_handle_t node, cap_t cap)
{
	uint64_t pid = current->regs.a2;
	if (pid >= PROC_COUNT)
		return EXCPT_MPID;
	proc_t *proc = proc_get(pid);
	switch (sysnr) {
	case SYSCALL_MONITOR_SUSPEND: {
		proc_suspend(proc);
		return EXCPT_NONE;
	}
	case SYSCALL_MONITOR_RESUME: {
		proc_resume(proc);
		return EXCPT_NONE;
	}
	case SYSCALL_MONITOR_GET_REG: {
		uint64_t expct_state = proc->state & PSF_SUSPEND;
		uint64_t *regs = (uint64_t *)&proc->regs;
		uint64_t reg = current->regs.a3;
		if (reg >= REG_COUNT)
			return EXCPT_INDEX;
		if (!proc_acquire(proc, expct_state))
			return EXCPT_MBUSY;
		current->regs.a1 = regs[reg];
		proc_release(proc);
		return EXCPT_NONE;
	}
	case SYSCALL_MONITOR_SET_REG: {
		uint64_t expct_state = proc->state & PSF_SUSPEND;
		uint64_t *regs = (uint64_t *)&proc->regs;
		uint64_t reg = current->regs.a3;
		uint64_t value = current->regs.a4;
		if (reg >= REG_COUNT)
			return EXCPT_INDEX;
		if (!proc_acquire(proc, expct_state))
			return EXCPT_MBUSY;
		regs[reg] = value;
		proc_release(proc);
		return EXCPT_NONE;
	}
	case SYSCALL_MONITOR_READ_CAP: {
		uint64_t expct_state = proc->state & PSF_SUSPEND;
		uint64_t cidx = current->regs.a3;
		if (cidx >= CAP_COUNT)
			return EXCPT_INDEX;
		if (!proc_acquire(proc, expct_state))
			return EXCPT_MBUSY;
		cnode_handle_t read_cnode = cnode_get(pid, cidx);
		cap_t read_cap = cnode_cap(read_cnode);
		current->regs.a1 = read_cap.raw;
		proc_release(proc);
		return EXCPT_NONE;
	}
	case SYSCALL_MONITOR_TAKE_CAP:
	case SYSCALL_MONITOR_GIVE_CAP:
	case SYSCALL_MONITOR_PMP_SET:
	case SYSCALL_MONITOR_PMP_CLEAR:
	default:
		return EXCPT_UNIMPLEMENTED;
	}
}

uint64_t syscall_invoke_socket(uint64_t sysnr, cnode_handle_t node, cap_t cap)
{
	switch (sysnr) {
	case SYSCALL_SOCKET_RECV:
	case SYSCALL_SOCKET_SEND:
	case SYSCALL_SOCKET_SENDRECV:
	default:
		return EXCPT_UNIMPLEMENTED;
	}
}

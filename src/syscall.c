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
#include "timer.h"
#include "trap.h"

// static proc_t *_listeners[NCHANNEL];

register proc_t *current __asm__("tp");

static void syscall_exception(excpt_t error)
{
	preemption_disable();
	current->regs.a0 = error;
	current->regs.pc += 4;
	preemption_enable();
}

static void syscall_getinfo(uint64_t nr)
{
	uint64_t val;
	switch (nr) {
	case 0:
		val = current->pid;
		break;
	default:
		val = 0;
	}
	preemption_disable();
	current->regs.a0 = val;
	current->regs.pc += 4;
	preemption_enable();
}

static void syscall_getreg(uint64_t reg)
{
	uint64_t val;
	uint64_t *regs = (uint64_t *)&current->regs;
	if (reg < REG_COUNT) {
		val = regs[reg];
	} else {
		val = 0;
	}
	preemption_disable();
	current->regs.a0 = val;
	current->regs.pc += 4;
	preemption_enable();
}

static void syscall_setreg(uint64_t reg, uint64_t val)
{
	uint64_t *regs = (uint64_t *)&current->regs;
	preemption_disable();
	if (reg < REG_COUNT)
		regs[reg] = val;
	current->regs.pc += 4;
	preemption_enable();
}

static void syscall_yield(void)
{
	schedule_yield();
}

static void syscall_getcap(uint64_t idx)
{
	if (idx >= NCAP) {
		syscall_exception(EXCPT_INDEX);
		return;
	}
	cnode_handle_t node = cnode_get(current->pid, idx);
	cap_t cap = cnode_cap(node);
	preemption_disable();
	current->regs.a0 = cap.raw;
	current->regs.pc += 4;
	preemption_enable();
}

static void syscall_movcap(uint64_t src_idx, uint64_t dst_idx)
{
	if (src_idx >= NCAP || dst_idx >= NCAP) {
		syscall_exception(EXCPT_INDEX);
		return;
	}
	cnode_handle_t src = cnode_get(current->pid, src_idx);
	cnode_handle_t dst = cnode_get(current->pid, dst_idx);

	preemption_disable();
	current->regs.a0 = cnode_move(src, dst);
	current->regs.pc += 4;
	preemption_enable();
}

static void syscall_delcap(uint64_t idx)
{
	if (idx >= NCAP) {
		syscall_exception(EXCPT_INDEX);
		return;
	}
	cnode_handle_t node = cnode_get(current->pid, idx);
	preemption_disable();
	current->regs.a0 = cnode_delete(node);
	current->regs.pc += 4;
	preemption_enable();
}

static void syscall_drvcap(uint64_t src_idx, uint64_t drv_idx, uint64_t cap_raw)
{
	if (src_idx >= NCAP || drv_idx >= NCAP) {
		syscall_exception(EXCPT_INDEX);
		return;
	}

	cnode_handle_t src = cnode_get(current->pid, src_idx);
	cnode_handle_t drv = cnode_get(current->pid, drv_idx);

	cap_t src_cap = cnode_cap(src);
	cap_t drv_cap = (cap_t){ .raw = cap_raw };

	if (!cap_can_derive(src_cap, drv_cap)) {
		syscall_exception(EXCPT_DERIVATION);
		return;
	}

	if (cap_get_type(src_cap) == CAPTY_TIME) {
		cap_time_set_free(&src_cap, cap_time_get_begin(drv_cap));
	} else if (cap_get_type(src_cap) == CAPTY_MEMORY
		   && cap_get_type(drv_cap) == CAPTY_MEMORY) {
		cap_memory_set_free(&src_cap, cap_memory_get_begin(drv_cap));
	} else if (cap_get_type(src_cap) == CAPTY_MEMORY
		   && cap_get_type(drv_cap) == CAPTY_PMP) {
		cap_memory_set_lock(&src_cap, 1);
	} else if (cap_get_type(src_cap) == CAPTY_CHANNEL
		   && cap_get_type(drv_cap) == CAPTY_CHANNEL) {
		cap_channel_set_free(&src_cap, cap_channel_get_begin(drv_cap));
	} else if (cap_get_type(src_cap) == CAPTY_CHANNEL
		   && cap_get_type(drv_cap) == CAPTY_SOCKET) {
		cap_channel_set_free(&src_cap,
				     cap_socket_get_channel(drv_cap) + 1);
	} else if (cap_get_type(src_cap) == CAPTY_MONITOR) {
		cap_monitor_set_free(&src_cap, cap_monitor_get_begin(drv_cap));
	} else if (cap_get_type(src_cap) == CAPTY_SOCKET) {
		cap_socket_set_listeners(&src_cap,
					 cap_socket_get_listeners(src_cap) + 1);
	}

	preemption_disable();
	excpt_t status = cnode_update(src, src_cap);
	if (status == EXCPT_NONE) {
		status = cnode_insert(drv, drv_cap, src);
	}
	current->regs.a0 = status;
	current->regs.pc += 4;
	preemption_enable();
}

static void syscall_revcap(uint64_t idx)
{
	if (idx >= NCAP) {
		syscall_exception(EXCPT_INDEX);
		return;
	}

	// If preempted, return with preempted error msg.
	syscall_exception(EXCPT_PREEMPTED);

	cnode_handle_t node = cnode_get(current->pid, idx);
	cap_t cap = cnode_cap(node);

	while (cnode_is_null(node)) {
		cnode_handle_t next_node = cnode_next(node);
		cap_t next_cap = cnode_cap(next_node);
		if (!cap_is_child(cap, next_cap))
			break;
		preemption_disable();
		cnode_delete_if(next_node, next_cap, node);
		preemption_enable();
	}

	switch (cap_get_type(cap)) {
	case CAPTY_TIME:
		cap_time_set_free(&cap, cap_time_get_begin(cap));
		break;
	case CAPTY_MEMORY:
		cap_memory_set_free(&cap, cap_memory_get_begin(cap));
		cap_memory_set_lock(&cap, 0);
		break;
	case CAPTY_CHANNEL:
		cap_channel_set_free(&cap, cap_channel_get_begin(cap));
		break;
	case CAPTY_MONITOR:
		cap_monitor_set_free(&cap, cap_monitor_get_begin(cap));
		break;
	case CAPTY_SOCKET:
		cap_socket_set_listeners(&cap, 0);
		break;
	default:
		current->regs.a0 = EXCPT_NONE;
		return;
	}

	preemption_disable();
	//! No updating PC
	current->regs.a0 = cnode_update(node, cap);
	preemption_enable();
}

/**
 * Use a PMP capability to load its configuration to a certain
 * pmp slot.
 *
 * Notes:
 * We set the pmp config before updating the associated capability. Why?
 * Suppose thread A update the PMP capability X, then set pmpcfg.
 * Thread B is doing a revoke operationg, deleting X, then unsetting the config
 * if X was used at the point of deletion. We have the following scenario:
 * - Thread A: Updates X, now X says that pmpcfg_i is used.
 * - Thread B: Deletes X, sees that pmpcfg_i was used by X.
 * - Thread B: Unsets pmpcfg_i.
 * - Thread A: Sets pmpcfg_i.
 * Now we have a scenario where pmpcfg_i is set without an associated PMP
 * capability.
 *
 * By setting the PMP configuration first, we can revert when we notice that the
 * update has failed. Thus after the operation has finished (more specifically,
 * in a quiescent period, pmpcfg_i is never set unless there is an associated
 * PMP capability.
 *
 * Lets look at above scenario using this method:
 * - Thread A: Sets pmpcfg_i.
 * - Thread A: Updates X, now X says that pmpcfg_i is used.
 * - Thread B: Deletes X, sees that pmpcfg_i was used by X.
 * - Thread B: Unsets pmpcfg_i.
 * Or
 * - Thread A: Sets pmpcfg_i.
 * - Thread B: Deletes X, sees that pmpcfg_i was used by X.
 * - Thread A: Attempts updates X, fails because X has been deleted.
 * - Thread A: Unsets pmpcfg_i
 * The moment thread B unsets pmpcfg_i is irrelevant as both A and B
 * unsets pmpcfg_i as a last action.
 */
static void syscall_invpmp_load(cnode_handle_t node, cap_t cap)
{
	// Used - Set if PMP capability is being used.
	uint64_t used = cap_pmp_get_used(cap);
	// Index - The PMP configuration index.
	uint64_t idx = current->regs.a2 % PMP_COUNT;

	// Check if pmp capability is already being used.
	if (used) {
		// If used, we have a collision exception
		syscall_exception(EXCPT_COLLISION);
		return;
	}

	// Check if pmpcfg[idx] is begin used.
	if (current->pmpcfg[idx]) {
		// If used, we have a collision exception
		syscall_exception(EXCPT_COLLISION);
		return;
	}

	// Set used and idx fields.
	cap_pmp_set_used(&cap, 1);
	cap_pmp_set_idx(&cap, idx);

	// Disable preemption!!!
	preemption_disable();

	// Optimistically update the pmp config
	current->pmpaddr[idx] = cap_pmp_get_addr(cap);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	current->pmpcfg[idx] = cap_pmp_get_rwx(cap) | 0x18;

	// Update the node
	excpt_t status = cnode_update(node, cap);

	// If update fails, then node was deleted, revert the pmp config update
	if (status != EXCPT_NONE) {
		current->pmpcfg[idx] = 0;
	}

	// Update registers
	current->regs.a0 = status;
	current->regs.pc += 4;

	// Enable preemption!!!
	preemption_enable();
}

static void syscall_invpmp_unload(cnode_handle_t node, cap_t cap)
{
	// Used - Set if PMP capability is being used.
	uint64_t used = cap_pmp_get_used(cap);
	// Index - The PMP configuration index.
	uint64_t idx = cap_pmp_get_idx(cap);

	// If not used, then nothing to unload.
	if (!used) {
		syscall_exception(EXCPT_NONE);
		return;
	}

	// Unset used and idx fields
	cap_pmp_set_used(&cap, 0);
	cap_pmp_set_idx(&cap, 0);

	// Disable preemption!!!
	preemption_disable();

	// Update node
	excpt_t status = cnode_update(node, cap);

	// If update was a success, then set pmpconf[idx] to 0.
	if (status == EXCPT_NONE) {
		current->pmpcfg[idx] = 0;
	}

	// Update registers
	current->regs.a0 = status;
	current->regs.pc += 4;

	// Enable preemption!!!
	preemption_enable();
}

static void syscall_invpmp(cnode_handle_t node, cap_t cap)
{
	kassert(cap.type == CAPTY_PMP);
	uint64_t inv = current->regs.a1;
	switch (inv) {
	case 0:
		syscall_invpmp_load(node, cap);
		break;
	case 1:
		syscall_invpmp_unload(node, cap);
		break;
	default:
		syscall_exception(EXCPT_UNIMPLEMENTED);
		break;
	}
}

static void syscall_invsocket(cnode_handle_t node, cap_t cap)
{
	kassert(cap.type == CAPTY_SOCKET);
	syscall_exception(EXCPT_UNIMPLEMENTED);
}

static void syscall_invmonitor(cnode_handle_t node, cap_t cap)
{
	kassert(cap.type == CAPTY_MONITOR);
	syscall_exception(EXCPT_UNIMPLEMENTED);
}

static void syscall_invcap(uint64_t idx)
{
	if (idx >= NCAP) {
		syscall_exception(EXCPT_INDEX);
		return;
	}

	cnode_handle_t node = cnode_get(current->pid, idx);
	cap_t cap = cnode_cap(node);

	switch (cap_get_type(cap)) {
	case CAPTY_NONE:
		syscall_exception(EXCPT_EMPTY);
		break;
	case CAPTY_PMP:
		syscall_invpmp(node, cap);
		break;
	case CAPTY_SOCKET:
		syscall_invsocket(node, cap);
		break;
	case CAPTY_MONITOR:
		syscall_invmonitor(node, cap);
		break;
	default:
		syscall_exception(EXCPT_UNIMPLEMENTED);
		break;
	}
}

void syscall_handler(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
		     uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7)
{
	switch (a7) {
	case SYSCALL_GETINFO:
		syscall_getinfo(a0);
		break;
	case SYSCALL_GETREG:
		syscall_getreg(a0);
		break;
	case SYSCALL_SETREG:
		syscall_setreg(a0, a1);
		break;
	case SYSCALL_YIELD:
		syscall_yield();
		break;
	case SYSCALL_GETCAP:
		syscall_getcap(a0);
		break;
	case SYSCALL_MOVCAP:
		syscall_movcap(a0, a1);
		break;
	case SYSCALL_DELCAP:
		syscall_delcap(a0);
		break;
	case SYSCALL_DRVCAP:
		syscall_drvcap(a0, a1, a2);
		break;
	case SYSCALL_REVCAP:
		syscall_revcap(a0);
		break;
	case SYSCALL_INVCAP:
		syscall_invcap(a0);
		break;
	default:
		syscall_exception(EXCPT_UNIMPLEMENTED);
	}
}

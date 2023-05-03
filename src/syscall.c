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
	current->regs[REG_A0] = error;
	current->regs[REG_PC] += 4;
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
	current->regs[REG_A0] = val;
	current->regs[REG_PC] += 4;
        preemption_enable();
}

static void syscall_getreg(uint64_t reg)
{
        uint64_t val;
        if (reg < REG_COUNT) {
                val = current->regs[reg];
        } else {
                val = 0;
        }
        preemption_disable();
	current->regs[REG_A0] = val;
	current->regs[REG_PC] += 4;
        preemption_enable();
}

static void syscall_setreg(uint64_t reg, uint64_t val)
{
        preemption_disable();
        if (reg < REG_COUNT)
                current->regs[reg] = val;
        current->regs[REG_PC] += 4;
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
	current->regs[REG_A0] = cap.raw;
	current->regs[REG_PC] += 4;
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
	current->regs[REG_A0] = cnode_move(src, dst);
	current->regs[REG_PC] += 4;
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
	current->regs[REG_A0] = cnode_delete(node);
	current->regs[REG_PC] += 4;
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
	current->regs[REG_A0] = cnode_update(src, src_cap);
	if (current->regs[REG_A0] == EXCPT_NONE) {
		current->regs[REG_A0] = cnode_insert(drv, drv_cap, src);
	}
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
	default:
		current->regs[REG_A0] = EXCPT_NONE;
		return;
	}

	preemption_disable();
	//! No updating PC
	current->regs[REG_A0] = cnode_update(node, cap);
	preemption_enable();
}

static void syscall_invpmp_load(cnode_handle_t node, cap_t cap)
{
        uint64_t used = cap_pmp_get_used(cap);
        uint64_t idx = current->regs[REG_A2] % 0x8;
        if (used) {
                syscall_exception(EXCPT_COLLISION);
                return;
        }
        cap_pmp_set_used(&cap, 1);
        cap_pmp_set_idx(&cap, idx);
        preemption_disable();
        current->regs[REG_A0] = cnode_update(node, cap);
        if (current->regs[REG_A0] == EXCPT_NONE) {
                current->pmpconf[idx] = cap_pmp_get_addr(cap) << 8 | cap_pmp_get_rwx(cap) | 0x18;
        }
        preemption_enable();
}

static void syscall_invpmp_unload(cnode_handle_t node, cap_t cap)
{
        uint64_t used = cap_pmp_get_used(cap);
        uint64_t idx = cap_pmp_get_idx(cap);
        if (!used) {
                syscall_exception(EXCPT_COLLISION);
                return;
        }
        cap_pmp_set_used(&cap, 0);
        cap_pmp_set_idx(&cap, 0);
        preemption_disable();
        current->regs[REG_A0] = cnode_update(node, cap);
        if (current->regs[REG_A0] == EXCPT_NONE) {
                current->pmpconf[idx] = 0;
        }
        preemption_enable();
}

static void syscall_invpmp(cnode_handle_t node, cap_t cap)
{
	kassert(cap.type == CAPTY_PMP);
        uint64_t inv = current->regs[REG_A1];

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

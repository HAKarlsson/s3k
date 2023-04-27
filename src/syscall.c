/* See LICENSE file for copyright and license details. */
#include "syscall.h"

#include "cap.h"
#include "common.h"
#include "consts.h"
#include "csr.h"
#include "kassert.h"
#include "schedule.h"
#include "ticket_lock.h"
#include "timer.h"
#include "trap.h"

// static proc_t *_listeners[NCHANNEL];

register proc_t *current __asm__("tp");

void syscall_getinfo(void)
{
}

void syscall_getreg(void)
{
}

void syscall_setreg(void)
{
}

void syscall_yield(void)
{
}

void syscall_getcap(uint64_t idx)
{
	if (idx >= NCAP) {
		current->regs[REG_A0] = EXCPT_INDEX;
		return;
	}
	cap_t cap = cnode_cap(cnode_idx(current->pid, idx));
	current->regs[REG_A0] = EXCPT_NONE;
	current->regs[REG_A1] = cap.raw;
}

void syscall_movcap(uint64_t src_idx, uint64_t dst_idx)
{
	if (src_idx >= NCAP || dst_idx >= NCAP) {
		current->regs[REG_A0] = EXCPT_INDEX;
		return;
	}
	src_idx = cnode_idx(current->pid, src_idx);
	dst_idx = cnode_idx(current->pid, dst_idx);

	current->regs[REG_A0] = cnode_move(src_idx, dst_idx);
}

void syscall_delcap(uint64_t idx)
{
	if (idx >= NCAP) {
		current->regs[REG_A0] = EXCPT_INDEX;
		return;
	}
	idx = cnode_idx(current->pid, idx);
	current->regs[REG_A0] = cnode_delete(idx);
}

static void syscall_drvtime(uint64_t src_idx, cap_t cap, uint64_t drv_idx,
			    cap_t drv_cap)
{
	kassert(cap.type == CAPTY_TIME);
	if (!cap_time_derive(cap, drv_cap)) {
		current->regs[REG_A0] = EXCPT_DERIVATION;
		return;
	}
}

static void syscall_drvmemory(uint64_t src_idx, cap_t cap, uint64_t drv_idx,
			      cap_t drv_cap)
{
	kassert(cap.type == CAPTY_MEMORY);
}

static void syscall_drvchannel(uint64_t src_idx, cap_t cap, uint64_t drv_idx,
			       cap_t drv_cap)
{
	kassert(cap.type == CAPTY_CHANNEL);
}

static void syscall_drvsocket(uint64_t src_idx, cap_t cap, uint64_t drv_idx,
			      cap_t drv_cap)
{
	kassert(cap.type == CAPTY_SOCKET);
}

static void syscall_drvmonitor(uint64_t src_idx, cap_t cap, uint64_t drv_idx,
			       cap_t drv_cap)
{
	kassert(cap.type == CAPTY_MONITOR);
}

void syscall_drvcap(uint64_t src_idx, uint64_t drv_idx, uint64_t cap_raw)
{
	if (src_idx >= NCAP || drv_idx >= NCAP) {
		current->regs[REG_A0] = EXCPT_INDEX;
		return;
	}
	src_idx = cnode_idx(current->pid, src_idx);
	drv_idx = cnode_idx(current->pid, drv_idx);

	cap_t src_cap = cnode_cap(src_idx);
	cap_t drv_cap = (cap_t){ .raw = cap_raw };
	switch (src_cap.type) {
	case CAPTY_NONE:
		current->regs[REG_A0] = EXCPT_EMPTY;
		break;
	case CAPTY_TIME:
		syscall_drvtime(src_idx, src_cap, drv_idx, drv_cap);
		break;
	case CAPTY_MEMORY:
		syscall_drvmemory(src_idx, src_cap, drv_idx, drv_cap);
		break;
	case CAPTY_CHANNEL:
		syscall_drvchannel(src_idx, src_cap, drv_idx, drv_cap);
		break;
	case CAPTY_SOCKET:
		syscall_drvsocket(src_idx, src_cap, drv_idx, drv_cap);
		break;
	case CAPTY_MONITOR:
		syscall_drvmonitor(src_idx, src_cap, drv_idx, drv_cap);
		break;
	default:
		current->regs[REG_A0] = EXCPT_UNIMPLEMENTED;
	}
}

static void syscall_revtime(uint64_t idx, cap_t cap)
{
	kassert(cap.type == CAPTY_TIME);
}

static void syscall_revmemory(uint64_t idx, cap_t cap)
{
	kassert(cap.type == CAPTY_MEMORY);
}

static void syscall_revchannel(uint64_t idx, cap_t cap)
{
	kassert(cap.type == CAPTY_CHANNEL);
}

static void syscall_revsocket(uint64_t idx, cap_t cap)
{
	kassert(cap.type == CAPTY_SOCKET);
}

static void syscall_revmonitor(uint64_t idx, cap_t cap)
{
	kassert(cap.type == CAPTY_MONITOR);
}

void syscall_revcap(uint64_t idx)
{
	if (idx >= NCAP) {
		current->regs[REG_A0] = EXCPT_INDEX;
		return;
	}
	idx = cnode_idx(current->pid, idx);
	cap_t cap = cnode_cap(idx);

	switch (cap.type) {
	case CAPTY_NONE:
		current->regs[REG_A0] = EXCPT_EMPTY;
		break;
	case CAPTY_TIME:
		syscall_revtime(idx, cap);
		break;
	case CAPTY_MEMORY:
		syscall_revmemory(idx, cap);
		break;
	case CAPTY_CHANNEL:
		syscall_revchannel(idx, cap);
		break;
	case CAPTY_SOCKET:
		syscall_revsocket(idx, cap);
		break;
	case CAPTY_MONITOR:
		syscall_revmonitor(idx, cap);
		break;
	default:
		current->regs[REG_A0] = EXCPT_UNIMPLEMENTED;
		break;
	}
}

static void syscall_invsocket(uint64_t idx, cap_t cap)
{
	kassert(cap.type == CAPTY_SOCKET);
}

static void syscall_invmonitor(uint64_t idx, cap_t cap)
{
	kassert(cap.type == CAPTY_MONITOR);
}

void syscall_invcap(uint64_t idx)
{
	if (idx >= NCAP) {
		current->regs[REG_A0] = EXCPT_INDEX;
		return;
	}

	idx = cnode_idx(current->pid, idx);
	cap_t cap = cnode_cap(idx);

	switch (cap.type) {
	case CAPTY_NONE:
		current->regs[REG_A0] = EXCPT_EMPTY;
		break;
	case CAPTY_SOCKET:
		syscall_invsocket(idx, cap);
		break;
	case CAPTY_MONITOR:
		syscall_invmonitor(idx, cap);
		break;
	default:
		current->regs[REG_A0] = EXCPT_UNIMPLEMENTED;
	}
}

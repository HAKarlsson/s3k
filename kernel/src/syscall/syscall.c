/* See LICENSE file for copyright and license details. */
#include "syscall/syscall.h"

#include "cap/table.h"
#include "csr.h"
#include "drivers/timer.h"
#include "kassert.h"
#include "kernel.h"
#include "macro.h"
#include "proc/current.h"
#include "proc/proc.h"
#include "sched/preemption.h"
#include "sched/sched.h"
#include "syscall/util.h"
#include "trap.h"

#define N_PMP 8

void handle_get_info(uint64_t info)
{
	switch (info) {
	case 0:
		current->regs.a0 = current->pid;
		break;
	case 1:
		current->regs.a0 = time_get();
		break;
	case 2:
		current->regs.a0 = current->end_time;
		break;
	case 3:
		current->regs.a0 = current->instrument_wcet;
		break;
	default:
		current->regs.a0 = 0;
	}
}

void handle_read_reg(uint64_t reg)
{
	if (check_invalid_register(reg))
		return;

	uint64_t *regs = (uint64_t *)&current->regs;
	current->regs.a0 = SUCCESS;
	current->regs.a1 = regs[reg];
}

void handle_write_reg(uint64_t reg, uint64_t val)
{
	if (check_invalid_register(reg))
		return;

	uint64_t *regs = (uint64_t *)&current->regs;

	regs[reg] = val;
	current->regs.a0 = SUCCESS;
}

void handle_sync(void)
{
}

void handle_mem_sync(void)
{
}

void handle_cap_read(uint64_t i)
{
	if (check_invalid_cslot(i))
		return;

	cidx_t src = cidx(current->pid, i);
	if (check_src_empty(src))
		return;

	current->regs.a0 = SUCCESS;
	current->regs.a1 = ctable_get_cap(src).raw;
}

void handle_cap_move(uint64_t i, uint64_t j)
{
	if (check_invalid_cslot(i) || check_invalid_cslot(j))
		return;

	cidx_t src = cidx(current->pid, i);
	cidx_t dst = cidx(current->pid, j);
	if (check_src_empty(src) || check_dst_occupied(dst))
		return;

	if (check_preemption())
		return;

	cap_t cap = ctable_get_cap(src);

	kernel_lock();
	if (!check_src_empty(src)) {
		move_cap(src, cap, dst);
		current->regs.a0 = SUCCESS;
	}
	kernel_unlock();
}

void handle_cap_delete(uint64_t i)
{
	cidx_t src = cidx(current->pid, i);
	// Validate capability index.
	if (check_invalid_cslot(i) || check_src_empty(src))
		return;

	if (check_preemption())
		return;

	cap_t cap = ctable_get_cap(src);

	kernel_lock();
	if (!check_src_empty(src)) {
		delete_cap(src, cap); /* Cleanup */
		current->regs.a0 = SUCCESS;
	}
	kernel_unlock();
}

void handle_cap_revoke(uint64_t i)
{
	// Validate capability index.
	if (check_invalid_cslot(i))
		return;

	cidx_t src = cidx(current->pid, i);
	if (check_src_empty(src))
		return;

	cap_t src_cap = ctable_get_cap(src);

	while (!ctable_is_empty(src)) {
		cidx_t next = ctable_get_next(src);
		cap_t next_cap = ctable_get_cap(next);

		if (!cap_can_revoke(src_cap, next_cap))
			break;

		if (check_preemption())
			return;

		kernel_lock();
		// Try delete next capability
		if (!check_src_empty(src)) {
			try_revoke_cap(src, src_cap, next, next_cap);
			src_cap = ctable_get_cap(src);
		}
		kernel_unlock();
	}

	if (check_src_empty(src) || check_preemption())
		return;

	kernel_lock();
	// Restore capability
	if (!check_src_empty(src)) {
		restore_cap(src, src_cap);
		current->regs.a0 = SUCCESS;
	}
	kernel_unlock();
}

void handle_cap_derive(uint64_t i, uint64_t j, uint64_t new_cap_raw)
{
	// Check if cslot is valid.
	if (check_invalid_cslot(i) || check_invalid_cslot(j))
		return;

	cidx_t src = cidx(current->pid, i);
	cidx_t dst = cidx(current->pid, j);
	if (check_src_empty(src) || check_dst_occupied(dst))
		return;

	cap_t src_cap = ctable_get_cap(src);
	cap_t new_cap = (cap_t){ .raw = new_cap_raw };

	if (check_invalid_derivation(src_cap, new_cap))
		return;

	if (check_preemption())
		return;

	kernel_lock();
	if (!check_src_empty(src)) {
		derive_cap(src, src_cap, dst, new_cap);
		current->regs.a0 = SUCCESS;
	}
	kernel_unlock();
}

void handle_pmp_load(uint64_t i, uint64_t pmpidx)
{
	if (check_invalid_cslot(i) || check_invalid_pmpidx(pmpidx))
		return;

	cidx_t src = cidx(current->pid, i);
	if (check_src_empty(src))
		return;

	if (check_pmp_occupied(current->pid, pmpidx))
		return;

	cap_t cap = ctable_get_cap(src);
	if (check_invalid_capty(cap, CAPTY_PMP) || check_pmp_used(cap))
		return;

	if (check_preemption())
		return;

	kernel_lock();
	if (!check_src_empty(src)) {
		pmp_load(src, cap, pmpidx);
		current->regs.a0 = SUCCESS;
	}
	kernel_unlock();
}

void handle_pmp_unload(uint64_t i)
{
	if (check_invalid_cslot(i))
		return;

	cidx_t src = cidx(current->pid, i);
	if (check_src_empty(src))
		return;

	cap_t cap = ctable_get_cap(src);
	if (check_invalid_capty(cap, CAPTY_PMP) || check_pmp_unused(cap))
		return;

	if (check_preemption())
		return;

	kernel_lock();
	if (!check_src_empty(src)) {
		pmp_unload(src, cap);
		current->regs.a0 = SUCCESS;
	}
	kernel_unlock();
}

void handle_invalid_syscall(void)
{
	current->regs.a0 = ERR_INVALID_SYSCALL;
}

void syscall_dispatch(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
		      uint64_t a4, uint64_t a5, uint64_t a6, uint64_t sysnr)
{
	current->regs.pc += 4;
	switch (sysnr) {
	case SYSCALL_GET_INFO:
		handle_get_info(a0);
		break;
	case SYSCALL_GET_REG:
		handle_read_reg(a0);
		break;
	case SYSCALL_SET_REG:
		handle_write_reg(a0, a1);
		break;
	case SYSCALL_SYNC:
		handle_sync();
		break;
	case SYSCALL_MEM_SYNC:
		handle_mem_sync();
		break;
	case SYSCALL_CAP_READ:
		handle_cap_read(a0);
		break;
	case SYSCALL_CAP_MOVE:
		handle_cap_move(a0, a1);
		break;
	case SYSCALL_CAP_DELETE:
		handle_cap_delete(a0);
		break;
	case SYSCALL_CAP_REVOKE:
		handle_cap_revoke(a0);
		break;
	case SYSCALL_CAP_DERIVE:
		handle_cap_derive(a0, a1, a2);
		break;
	case SYSCALL_PMP_LOAD:
		handle_pmp_load(a0, a1);
		break;
	case SYSCALL_PMP_UNLOAD:
		handle_pmp_unload(a0);
		break;
	default:
		handle_invalid_syscall();
		break;
	}
}

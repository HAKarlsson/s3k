/* See LICENSE file for copyright and license details. */
#include "syscall.h"

#include "cap_operations.h"
#include "cap_table.h"
#include "common.h"
#include "csr.h"
#include "current.h"
#include "kassert.h"
#include "preemption.h"
#include "proc.h"
#include "schedule.h"
#include "time.h"
#include "trap.h"

#define CURRENT_ARGS (&current->regs[REG_A0])

int syscall_get_info(uint64_t info)
{
	switch (info) {
	case 0:
		CURRENT_ARGS[0] = current->pid;
		return EXCPT_NONE;
	case 1:
		CURRENT_ARGS[0] = time_get();
		return EXCPT_NONE;
	case 2:
		CURRENT_ARGS[0] = current->end_time;
		return EXCPT_NONE;
#ifdef INSTRUMENTATION
	case 3:
		CURRENT_ARGS[0] = current->instrument_wcet;
		current->instrument_wcet = 0;
		return EXCPT_NONE;
#endif
	default:
		return EXCPT_ERROR;
	}
}

int syscall_get_reg(uint64_t regid)
{
	if (regid <= REG_T6 || regid >= NUM_OF_REGS)
		return EXCPT_REG_INDEX;

	CURRENT_ARGS[0] = current->regs[regid];
	return EXCPT_NONE;
}

int syscall_set_reg(uint64_t reg, uint64_t value)
{
	if (reg <= REG_T6 || reg >= NUM_OF_REGS)
		return EXCPT_REG_INDEX;

	current->regs[reg] = value;
	return EXCPT_NONE;
}

int syscall_yield(uint64_t until)
{
	// Yield.
	current->sleep = until ? until : current->end_time;
	current->regs[REG_PC] += 4;
	return EXCPT_PREEMPT;
}

int syscall_get_cap(uint64_t cidx)
{
	// Create and check that capability pointer is valid.
	cptr_t cptr = cptr_mk(current->pid, cidx);
	if (!cptr_is_valid(cptr))
		return EXCPT_INDEX;

	cap_t cap = ctable_get_cap(cptr);
	CURRENT_ARGS[0] = cap.raw;
	return EXCPT_NONE;
}

int syscall_move_cap(uint64_t src_cidx, uint64_t dest_cidx)
{
	// Create and check that capability pointers are valid.
	cptr_t src = cptr_mk(current->pid, src_cidx);
	cptr_t dst = cptr_mk(current->pid, dest_cidx);
	if (!cptr_is_valid(src) || !cptr_is_valid(dst))
		return EXCPT_INDEX;

	return cap_move(src, dst);
}

int syscall_delete_cap(uint64_t cidx)
{
	// Create and check that capability pointer is valid.
	cptr_t cptr = cptr_mk(current->pid, cidx);
	if (!cptr_is_valid(cptr))
		return EXCPT_INDEX;

	return cap_delete(cptr);
}

int syscall_revoke_cap(uint64_t cidx)
{
	// Create and check that capability pointer is valid.
	cptr_t cptr = cptr_mk(current->pid, cidx);
	if (!cptr_is_valid(cptr))
		return EXCPT_INDEX;

	return cap_revoke(cptr);
}

int syscall_derive_cap(uint64_t orig_cidx, uint64_t dest_cidx, uint64_t new_cap)
{
	// Create and check that capability pointers are valid.
	cptr_t orig = cptr_mk(current->pid, orig_cidx);
	cptr_t dest = cptr_mk(current->pid, dest_cidx);
	if (!cptr_is_valid(orig) || !cptr_is_valid(dest))
		return EXCPT_INDEX;

	return cap_derive(orig, dest, (cap_t){ .raw = new_cap });
}

int syscall_pmp_set(uint64_t pmp_cidx, uint64_t index)
{
	cptr_t cptr = cptr_mk(current->pid, pmp_cidx);
	if (!cptr_is_valid(cptr))
		return EXCPT_INDEX;

	return cap_pmp_set(cptr, index);
}

int syscall_pmp_clear(uint64_t pmp_cidx)
{
	cptr_t pmp_cptr = cptr_mk(current->pid, pmp_cidx);
	if (!cptr_is_valid(pmp_cptr))
		return EXCPT_INDEX;

	return cap_pmp_clear(pmp_cptr);
}

int syscall_monitor_suspend(uint64_t mon_cidx, uint64_t pid)
{
	cptr_t mon_cptr = cptr_mk(current->pid, mon_cidx);
	if (!cptr_is_valid(mon_cptr))
		return EXCPT_INDEX;

	return cap_monitor_suspend(mon_cptr, pid);
}

int syscall_monitor_resume(uint64_t mon_cidx, uint64_t pid)
{
	cptr_t mon_cptr = cptr_mk(current->pid, mon_cidx);
	if (!cptr_is_valid(mon_cptr))
		return EXCPT_INDEX;

	return cap_monitor_resume(mon_cptr, pid);
}

int syscall_monitor_get_reg(uint64_t mon_cidx, uint64_t pid, uint64_t reg)
{
	cptr_t mon_cptr = cptr_mk(current->pid, mon_cidx);
	if (!cptr_is_valid(mon_cptr))
		return EXCPT_INDEX;

	if (reg >= NUM_OF_REGS)
		return EXCPT_MONITOR_REG_INDEX;

	return cap_monitor_get_reg(mon_cptr, pid, reg, CURRENT_ARGS);
}

int syscall_monitor_set_reg(uint64_t mon_cidx, uint64_t pid, uint64_t reg, uint64_t val)
{
	cptr_t mon_cptr = cptr_mk(current->pid, mon_cidx);
	if (!cptr_is_valid(mon_cptr))
		return EXCPT_INDEX;

	if (reg >= NUM_OF_REGS)
		return EXCPT_MONITOR_REG_INDEX;

	return cap_monitor_set_reg(mon_cptr, pid, reg, val);
}

int syscall_monitor_get_cap(uint64_t mon_cidx, uint64_t pid, uint64_t src_cidx, uint64_t dest_cidx)
{
	cptr_t mon_cptr = cptr_mk(current->pid, mon_cidx);
	if (!cptr_is_valid(mon_cptr))
		return EXCPT_INDEX;

	cptr_t cptr = cptr_mk(pid, src_cidx);
	if (cptr < 0)
		return EXCPT_MONITOR_INDEX;

	return cap_monitor_get_cap(mon_cptr, pid, cptr, CURRENT_ARGS);
}

int syscall_monitor_take_cap(uint64_t mon_cidx, uint64_t pid, uint64_t src_cidx, uint64_t dest_cidx)
{
	cptr_t mon_cptr = cptr_mk(current->pid, mon_cidx);
	if (!cptr_is_valid(mon_cptr))
		return EXCPT_INDEX;

	cptr_t src_cptr = cptr_mk(pid, src_cidx);
	cptr_t dest_cptr = cptr_mk(current->pid, dest_cidx);
	if (!cptr_is_valid(src_cptr) || !cptr_is_valid(dest_cptr))
		return EXCPT_MONITOR_INDEX;

	return cap_monitor_move_cap(mon_cptr, pid, src_cptr, dest_cptr);
}

int syscall_monitor_give_cap(uint64_t mon_cidx, uint64_t pid, uint64_t src_cidx, uint64_t dest_cidx)
{
	cptr_t mon_cptr = cptr_mk(current->pid, mon_cidx);
	if (!cptr_is_valid(mon_cptr))
		return EXCPT_INDEX;

	cptr_t src_cptr = cptr_mk(current->pid, src_cidx);
	cptr_t dest_cptr = cptr_mk(pid, dest_cidx);
	if (!cptr_is_valid(src_cptr) || !cptr_is_valid(dest_cptr))
		return EXCPT_MONITOR_INDEX;

	return cap_monitor_move_cap(mon_cptr, pid, src_cptr, dest_cptr);
}

int syscall_monitor_pmp_set(uint64_t mon_cidx, uint64_t pid, uint64_t pmp_cidx, uint64_t index)
{
	cptr_t mon_cptr = cptr_mk(current->pid, mon_cidx);
	if (!cptr_is_valid(mon_cptr))
		return EXCPT_INDEX;

	cptr_t pmp_cptr = cptr_mk(pid, pmp_cidx);
	if (!cptr_is_valid(pmp_cidx))
		return EXCPT_MONITOR_INDEX;

	if (index >= NUM_OF_PMP)
		return EXCPT_PMP_INDEX;

	return cap_monitor_pmp_set(mon_cptr, pid, pmp_cptr, index);
}

int syscall_monitor_pmp_clear(uint64_t mon_cidx, uint64_t pid, uint64_t pmp_cidx)
{
	cptr_t mon_cptr = cptr_mk(current->pid, mon_cidx);
	if (!cptr_is_valid(mon_cptr))
		return EXCPT_INDEX;

	cptr_t pmp_cptr = cptr_mk(pid, pmp_cidx);
	if (!cptr_is_valid(pmp_cidx))
		return EXCPT_MONITOR_INDEX;

	return cap_monitor_pmp_clear(mon_cptr, pid, pmp_cptr);
}

int syscall_socket_send(uint64_t sock_cidx, uint64_t buf0, uint64_t buf1,
			uint64_t buf2, uint64_t buf3, uint64_t buf_cidx)
{
	cptr_t sock_cptr = cptr_mk(current->pid, sock_cidx);
	cptr_t buf_cptr = cptr_mk(current->pid, sock_cidx);
	uint64_t buf[] = { buf0, buf1, buf2, buf3 };
	if (!cptr_is_valid(sock_cptr))
		return EXCPT_INDEX;
	return cap_socket_send(sock_cptr, buf, buf_cptr);
}

int syscall_socket_recv(uint64_t sock_cidx)
{
	cptr_t sock_cptr = cptr_mk(current->pid, sock_cidx);
	if (!cptr_is_valid(sock_cptr))
		return EXCPT_INDEX;
	return cap_socket_recv(sock_cptr);
}

int syscall_socket_sendrecv(uint64_t sock_cidx, uint64_t buf0, uint64_t buf1,
			    uint64_t buf2, uint64_t buf3, uint64_t buf_cidx)
{
	cptr_t sock_cptr = cptr_mk(current->pid, sock_cidx);
	cptr_t buf_cptr = cptr_mk(current->pid, buf_cidx);
	uint64_t buf[] = { buf0, buf1, buf2, buf3 };
	if (!cptr_is_valid(sock_cptr))
		return EXCPT_INDEX;
	return cap_socket_sendrecv(sock_cptr, buf, buf_cptr);
}

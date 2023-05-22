/* See LICENSE file for copyright and license details. */
#include "syscall.h"

#include "cap_operations.h"
#include "cap_table.h"
#include "common.h"
#include "csr.h"
#include "current.h"
#include "excpt.h"
#include "kassert.h"
#include "preemption.h"
#include "proc.h"
#include "schedule.h"
#include "time.h"
#include "trap.h"

#define CURRENT_ARGS (&current->regs[REG_A0])

excpt_t syscall_get_info(uint64_t info)
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
	default:
		return EXCPT_ERROR;
	}
}

excpt_t syscall_get_reg(uint64_t regid)
{
	if (regid <= REG_T6 || regid >= NUM_OF_REGS)
		return EXCPT_REG_INDEX;

	CURRENT_ARGS[0] = current->regs[regid];
	return EXCPT_NONE;
}

excpt_t syscall_set_reg(uint64_t reg, uint64_t value)
{
	if (reg <= REG_T6 || reg >= NUM_OF_REGS)
		return EXCPT_REG_INDEX;
	CURRENT_ARGS[reg] = value;
	return EXCPT_NONE;
}

excpt_t syscall_yield(uint64_t until)
{
	// Yield.
	current->sleep = until ? until : current->end_time;
	schedule_yield();
	return EXCPT_PREEMPT;
}

excpt_t syscall_get_cap(uint64_t cidx)
{
	// Create and check that capability pointer is valid.
	cptr_t cptr = cptr_mk(current->pid, cidx);
	if (!cptr_is_valid(cptr))
		return EXCPT_INDEX;
	return cap_get_cap(cptr, CURRENT_ARGS);
}

excpt_t syscall_move_cap(uint64_t src_cidx, uint64_t dest_cidx)
{
	// Create and check that capability pointers are valid.
	cptr_t src = cptr_mk(current->pid, src_cidx);
	cptr_t dst = cptr_mk(current->pid, dest_cidx);
	if (!cptr_is_valid(src) || !cptr_is_valid(dst))
		return EXCPT_INDEX;
	return cap_move(src, dst);
}

excpt_t syscall_delete_cap(uint64_t cidx)
{
	// Create and check that capability pointer is valid.
	cptr_t cptr = cptr_mk(current->pid, cidx);
	if (!cptr_is_valid(cptr))
		return EXCPT_INDEX;
	return cap_delete(cptr);
}

excpt_t syscall_revoke_cap(uint64_t cidx)
{
	// Create and check that capability pointer is valid.
	cptr_t cptr = cptr_mk(current->pid, cidx);
	if (!cptr_is_valid(cptr))
		return EXCPT_INDEX;
	return cap_revoke(cptr);
}

excpt_t syscall_derive_cap(uint64_t orig_cidx, uint64_t dest_cidx, cap_t new_cap)
{
	// Create and check that capability pointers are valid.
	cptr_t orig = cptr_mk(current->pid, orig_cidx);
	cptr_t dest = cptr_mk(current->pid, dest_cidx);
	if (!cptr_is_valid(orig) || !cptr_is_valid(dest))
		return EXCPT_INDEX;
	return cap_derive(orig, dest, new_cap);
}

excpt_t syscall_pmp_set(uint64_t pmp_cidx, uint64_t index)
{
	cptr_t cptr = cptr_mk(current->pid, pmp_cidx);
	if (!cptr_is_valid(cptr))
		return EXCPT_INDEX;
	if (index >= NUM_OF_PMP)
		return EXCPT_ERROR;
	return cap_pmp_set(cptr, index);
}

excpt_t syscall_pmp_clear(uint64_t pmp_cidx)
{
	cptr_t cptr = cptr_mk(current->pid, pmp_cidx);
	if (!cptr_is_valid(cptr))
		return EXCPT_INDEX;
	return cap_pmp_clear(cptr);
}

// excpt_t syscall_invoke_monitor(uint64_t sysnr, proc_t *proc, uint64_t args[8], uint64_t ret[1])
//{
//	// Create and check if the capability pointer is valid
//	cptr_t mon = cptr_mk(proc->pid, args[0]);
//	if (!cptr_is_valid(mon))
//		return EXCPT_INDEX;
//
//	uint64_t pid = args[1];
//
//	// Dispatch the system call based on the provided sysnr
//	switch (sysnr) {
//	case SYSCALL_MONITOR_SUSPEND:
//		return cap_monitor_suspend(mon, pid);
//	case SYSCALL_MONITOR_RESUME:
//		return cap_monitor_resume(mon, pid);
//	case SYSCALL_MONITOR_GET_REG: {
//		uint64_t reg = args[2];
//		if (reg >= NUM_OF_REGS)
//			return EXCPT_MONITOR_REG_INDEX;
//		return cap_monitor_get_reg(mon, pid, reg, ret);
//	}
//	case SYSCALL_MONITOR_SET_REG: {
//		uint64_t reg = args[2];
//		uint64_t val = args[3];
//		if (reg >= NUM_OF_REGS)
//			return EXCPT_MONITOR_REG_INDEX;
//		return cap_monitor_set_reg(mon, pid, reg, val);
//	}
//	case SYSCALL_MONITOR_GET_CAP: {
//		uint64_t cidx = args[2];
//		if (cidx >= NUM_OF_CAPABILITIES)
//			return EXCPT_MONITOR_INDEX;
//		// Create and check if cptr is valid
//		return cap_monitor_get_cap(mon, pid, cidx, ret);
//	}
//	case SYSCALL_MONITOR_TAKE_CAP: {
//		uint64_t src_cidx = args[2];
//		uint64_t dest_cidx = args[3];
//		if (src_cidx >= NUM_OF_CAPABILITIES || dest_cidx >= NUM_OF_CAPABILITIES)
//			return EXCPT_MONITOR_INDEX;
//		return cap_monitor_move_cap(mon, pid, src_cidx, dest_cidx, true);
//	}
//	case SYSCALL_MONITOR_GIVE_CAP: {
//		uint64_t src_cidx = args[2];
//		uint64_t dest_cidx = args[3];
//		if (src_cidx >= NUM_OF_CAPABILITIES || dest_cidx >= NUM_OF_CAPABILITIES)
//			return EXCPT_MONITOR_INDEX;
//		return cap_monitor_move_cap(mon, pid, src_cidx, dest_cidx, false);
//	}
//	default:
//		kassert(0); // This should never be reached.
//	}
// }
//
// excpt_t syscall_invoke_socket(uint64_t sysnr, proc_t *proc, uint64_t args[8], uint64_t ret[1])
//{
//	return EXCPT_UNIMPLEMENTED;
// }

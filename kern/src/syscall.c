#include "syscall.h"

#include "csr.h"
#include "current.h"
#include "exception.h"
#include "ipc.h"
#include "lock.h"
#include "macro.h"
#include "mem.h"
#include "mon.h"
#include "preempt.h"
#include "proc.h"
#include "rtc.h"
#include "tsl.h"

/**
 * Get the current process's PID.
 */
static proc_result_t syscall_pid_get(pid_t pid, word_t args[8])
{
	(void)args;
	args[0] = pid;
	return proc_resume_result(current);
}

/**
 * Get a virtual register of the current process.
 */
static proc_result_t syscall_vreg_get(pid_t pid, word_t args[8])
{
	(void)pid;
	switch (args[1]) {
	case VREG_TPC:
		args[0] = current->trap.tpc;
		break;
	case VREG_TSP:
		args[0] = current->trap.tsp;
		break;
	case VREG_ECAUSE:
		args[0] = current->trap.ecause;
		break;
	case VREG_EVAL:
		args[0] = current->trap.eval;
		break;
	case VREG_EPC:
		args[0] = current->trap.epc;
		break;
	case VREG_ESP:
		args[0] = current->trap.esp;
		break;
	default:
		args[0] = 0;
		break;
	}
	return proc_resume_result(current);
}

/**
 * Set a virtual register of the current process.
 */
static proc_result_t syscall_vreg_set(pid_t pid, word_t args[8])
{
	(void)pid;
	switch (args[1]) {
	case VREG_TPC:
		current->trap.tpc = args[2];
		break;
	case VREG_TSP:
		current->trap.tsp = args[2];
		break;
	case VREG_ECAUSE:
		current->trap.ecause = args[2];
		break;
	case VREG_EVAL:
		current->trap.eval = args[2];
		break;
	case VREG_EPC:
		current->trap.epc = args[2];
		break;
	case VREG_ESP:
		current->trap.esp = args[2];
		break;
	default:
		break;
	}
	return proc_resume_result(current);
}

/**
 * Synchronize the current process's installed PMP without yielding.
 */
static proc_result_t syscall_sync(pid_t pid, word_t args[8])
{
	(void)pid;
	(void)args;
	return proc_reload_result(current);
}

/**
 * Sleep until the specified timeout, then call the scheduler.
 */
static proc_result_t syscall_sleep_until(pid_t pid, word_t args[8])
{
	if (args[1] != 0) {
		current->timeout = args[1];
	}
	proc_release(pid);
	return 0;
}

static int cap_introspect(pid_t pid, word_t type, index_t i, fuel_t offset, void *cap)
{
	switch (type) {
	case CAPTY_MEM:
		return mem_introspect(pid, i, offset, cap);
	case CAPTY_TSL:
		return tsl_introspect(pid, i, offset, cap);
	case CAPTY_MON:
		return mon_introspect(pid, i, offset, cap);
	case CAPTY_IPC:
		return ipc_introspect(pid, i, offset, cap);
	default:
		return ERR_INVALID_ARGUMENT;
	}
}

static proc_result_t syscall_cap_introspect(pid_t pid, word_t args[8])
{
	args[0] = cap_introspect(pid, args[1], args[2], args[3], &args[1]);
	return proc_resume_result(current);
}

static int cap_derive(pid_t owner, pid_t target, word_t type, index_t i, const word_t args[4])
{
	switch (type) {
	case CAPTY_MEM:
		return mem_derive(owner, i, target, args[0], args[1], args[2], args[3]);
	case CAPTY_TSL:
		return tsl_derive(owner, i, target, args[0], args[1], args[2]);
	case CAPTY_MON:
		return mon_derive(owner, i, target, args[0]);
	case CAPTY_IPC:
		return ipc_derive(owner, i, target, args[0], args[1], args[2]);
	default:
		return ERR_INVALID_ARGUMENT;
	}
}

static proc_result_t syscall_cap_derive(pid_t pid, word_t args[8])
{
	args[0] = cap_derive(pid, pid, args[1], args[2], &args[3]);
	return proc_resume_result(current);
}

static proc_result_t syscall_cap_revoke(pid_t pid, word_t args[8])
{
	switch (args[1]) {
	case CAPTY_MEM:
		args[0] = mem_revoke(pid, args[2]);
		break;
	case CAPTY_TSL:
		args[0] = tsl_revoke(pid, args[2]);
		break;
	case CAPTY_MON:
		args[0] = mon_revoke(pid, args[2]);
		break;
	case CAPTY_IPC:
		args[0] = ipc_revoke(pid, args[2]);
		break;
	default:
		args[0] = ERR_INVALID_ARGUMENT;
		break;
	}
	return proc_resume_result(current);
}

static proc_result_t syscall_cap_delete(pid_t pid, word_t args[8])
{
	switch (args[1]) {
	case CAPTY_MEM:
		args[0] = mem_delete(pid, args[2]);
		break;
	case CAPTY_TSL:
		args[0] = tsl_delete(pid, args[2]);
		break;
	case CAPTY_MON:
		args[0] = mon_delete(pid, args[2]);
		break;
	case CAPTY_IPC:
		args[0] = ipc_delete(pid, args[2]);
		break;
	default:
		args[0] = ERR_INVALID_ARGUMENT;
		break;
	}
	return proc_resume_result(current);
}

static proc_result_t syscall_mem_pmp_get(pid_t pid, word_t args[8])
{
	pmp_slot_t slot = 0;
	mem_perm_t rwx = 0;
	pmp_addr_t addr = 0;
	args[0] = mem_pmp_get(pid, args[1], &slot, &rwx, &addr);
	args[1] = slot;
	args[2] = rwx;
	args[3] = addr;
	return proc_resume_result(current);
}

static proc_result_t syscall_mem_pmp_set(pid_t pid, word_t args[8])
{
	args[0] = mem_pmp_set(pid, args[1], args[2], args[3]);
	return proc_resume_result(current);
}

/**
 * Enable or disable a time slice capability's minor frame.
 */
static proc_result_t syscall_tsl_set(pid_t pid, word_t args[8])
{
	args[0] = tsl_set(pid, args[1], args[2]);
	return proc_resume_result(current);
}

/**
 * Suspend the process that is being monitored by the specified monitor capability.
 */
static proc_result_t syscall_mon_suspend(pid_t pid, word_t args[8])
{
	args[0] = mon_suspend(pid, args[1]);
	return proc_resume_result(current);
}

/**
 * Resume the process that is being monitored by the specified monitor capability.
 */
static proc_result_t syscall_mon_resume(pid_t pid, word_t args[8])
{
	args[0] = mon_resume(pid, args[1]);
	return proc_resume_result(current);
}

/**
 * Yield execution time to the process being monitored by the specified monitor capability.
 */
static proc_result_t syscall_mon_yield(pid_t pid, word_t args[8])
{
	return mon_yield(pid, args[1], &args[0]);
}

/**
 * Get a register value of the process being monitored by the specified monitor capability.
 */
static proc_result_t syscall_mon_reg_get(pid_t pid, word_t args[8])
{
	word_t value;
	args[0] = mon_reg_get(pid, args[1], args[2], &value);
	args[1] = value;
	return proc_resume_result(current);
}

/**
 * Set a register of the process being monitored by the specified monitor capability.
 */
static proc_result_t syscall_mon_reg_set(pid_t pid, word_t args[8])
{
	args[0] = mon_reg_set(pid, args[1], args[2], args[3]);
	return proc_resume_result(current);
}

/**
 * Get a virtual register value of the process being monitored by the specified monitor capability.
 */
static proc_result_t syscall_mon_vreg_get(pid_t pid, word_t args[8])
{
	word_t value;
	args[0] = mon_vreg_get(pid, args[1], args[2], &value);
	args[1] = value;
	return proc_resume_result(current);
}

/**
 * Set a virtual register of the process being monitored by the specified monitor capability.
 */
static proc_result_t syscall_mon_vreg_set(pid_t pid, word_t args[8])
{
	args[0] = mon_vreg_set(pid, args[1], args[2], args[3]);
	return proc_resume_result(current);
}

/**
 * Get the state of the process being monitored by the specified monitor capability.
 */
static proc_result_t syscall_mon_state_get(pid_t pid, word_t args[8])
{
	word_t value;
	args[0] = mon_state_get(pid, args[1], &value);
	args[1] = value;
	return proc_resume_result(current);
}

static proc_result_t syscall_mon_cap_introspect(pid_t pid, word_t args[8])
{
	pid_t target = mon_get_pid(pid, args[1]);
	args[0] = ERR_INVALID_ACCESS;
	if (target != INVALID_PID) {
		args[0] = cap_introspect(target, args[2], args[3], args[4], &args[1]);
	}
	return proc_resume_result(current);
}

/**
 * Grant a capability to the process being monitored by the specified monitor capability.
 */
static proc_result_t syscall_mon_grant(pid_t pid, word_t args[8])
{
	args[0] = args[2] > CAPTY_IPC ? ERR_INVALID_ARGUMENT : mon_grant(pid, args[1], args[2], args[3]);
	return proc_resume_result(current);
}

static proc_result_t syscall_mon_cap_derive(pid_t pid, word_t args[8])
{
	pid_t target = mon_get_pid(pid, args[1]);
	args[0] = ERR_INVALID_ACCESS;
	if (target != INVALID_PID) {
		args[0] = cap_derive(pid, target, args[2], args[3], &args[4]);
	}
	return proc_resume_result(current);
}

static proc_result_t syscall_mon_mem_pmp_get(pid_t pid, word_t args[8])
{
	pid_t target = mon_get_pid(pid, args[1]);
	args[0] = ERR_INVALID_ACCESS;
	if (target != INVALID_PID) {
		pmp_slot_t slot = 0;
		mem_perm_t rwx = 0;
		pmp_addr_t addr = 0;
		args[0] = mem_pmp_get(target, args[2], &slot, &rwx, &addr);
		args[1] = slot;
		args[2] = rwx;
		args[3] = addr;
	}
	return proc_resume_result(current);
}

static proc_result_t syscall_mon_mem_pmp_set(pid_t pid, word_t args[8])
{
	pid_t target = mon_get_pid(pid, args[1]);
	args[0] = ERR_INVALID_ACCESS;
	if (target != INVALID_PID) {
		args[0] = mem_pmp_set(target, args[2], args[3], args[4]);
	}
	return proc_resume_result(current);
}

/**
 * Grant a time slice capability to the process being monitored by the specified monitor capability.
 */
static proc_result_t syscall_mon_tsl_set(pid_t pid, word_t args[8])
{
	pid_t target = mon_get_pid(pid, args[1]);
	args[0] = ERR_INVALID_ACCESS;
	if (target != INVALID_PID) {
		args[0] = tsl_set(target, args[2], args[3]);
	}
	return proc_resume_result(current);
}

/**
 * Send an synchronous IPC message in a unidirectional IPC channel.
 */
static proc_result_t syscall_ipc_send(pid_t pid, word_t args[8])
{
	word_t data[2] = {args[2], args[3]};
	return ipc_send(pid, args[1], data, args[4], args[5], &args[0]);
}

/**
 * Wait to receive an IPC message (synchronous, bidirectional/unidirectional IPC).
 */
static proc_result_t syscall_ipc_recv(pid_t pid, word_t args[8])
{
	return ipc_recv(pid, args[1], args[2], &args[0]);
}

/**
 * Make an IPC call (synchronous, bidirectional IPC channel) to an IPC server.
 */
static proc_result_t syscall_ipc_call(pid_t pid, word_t args[8])
{
	word_t data[2] = {args[2], args[3]};
	return ipc_call(pid, args[1], data, args[4], args[5], &args[0]);
}

/**
 * Send a reply to an IPC call.
 */
static proc_result_t syscall_ipc_reply(pid_t pid, word_t args[8])
{
	word_t data[2] = {args[2], args[3]};
	return ipc_reply(pid, args[1], data, args[4], args[5], &args[0]);
}

/**
 * Send a reply for an IPC call, then atomically wait to receive an IPC message.
 */
static proc_result_t syscall_ipc_replyrecv(pid_t pid, word_t args[8])
{
	word_t data[2] = {args[2], args[3]};
	return ipc_replyrecv(pid, args[1], data, args[4], args[5], args[6], &args[0]);
}

/**
 * Send an asynchronous IPC message in a unidirectional IPC channel.
 */
static proc_result_t syscall_ipc_asend(pid_t pid, word_t args[8])
{
	return ipc_asend(pid, args[1], args[2], &args[0]);
}

/**
 * Read the message inbox of an asynchronous IPC channel.
 */
static proc_result_t syscall_ipc_arecv(pid_t pid, word_t args[8])
{
	word_t data;
	args[0] = ipc_arecv(pid, args[1], &data);
	args[1] = data;
	return proc_resume_result(current);
}

/**
 * Handlers return zero to wait, a positive PCB to resume, or its negative to reload PMP.
 */
typedef proc_result_t (*handler_t)(pid_t pid, word_t args[8]);

/**
 * Handlers for individual system calls.
 */
static const handler_t handlers[] = {
	syscall_pid_get,
	syscall_vreg_get,
	syscall_vreg_set,
	syscall_sync,
	syscall_sleep_until,
	syscall_cap_introspect,
	syscall_cap_derive,
	syscall_cap_revoke,
	syscall_cap_delete,
	syscall_mem_pmp_get,
	syscall_mem_pmp_set,
	syscall_tsl_set,
	syscall_mon_suspend,
	syscall_mon_resume,
	syscall_mon_yield,
	syscall_mon_reg_get,
	syscall_mon_reg_set,
	syscall_mon_vreg_get,
	syscall_mon_vreg_set,
	syscall_mon_state_get,
	syscall_mon_cap_introspect,
	syscall_mon_grant,
	syscall_mon_cap_derive,
	syscall_mon_mem_pmp_get,
	syscall_mon_mem_pmp_set,
	syscall_mon_tsl_set,
	syscall_ipc_send,
	syscall_ipc_recv,
	syscall_ipc_call,
	syscall_ipc_reply,
	syscall_ipc_replyrecv,
	syscall_ipc_asend,
	syscall_ipc_arecv,
};

/**
 * System call handler.
 */
proc_result_t syscall_handler(void)
{
	// The system call number.
	word_t syscall_nr = current->regs.a0;

	// If system call number is invalid, make an exception.
	if (syscall_nr >= ARRAY_SIZE(handlers)) {
		return exception_handler(0x8, syscall_nr);
	}

	handler_t handler = handlers[syscall_nr];
	lock_acquire();
	proc_t *caller = current;
	if (preempt()) {
		proc_release(caller->pid);
		lock_release();
		return 0;
	}

	// Advance the program counter.
	caller->regs.pc += 4;

	// Call the system call handler
	proc_result_t result = handler(caller->pid, &caller->regs.a0);

	// Releases the lock.
	lock_release();

	return result;
}

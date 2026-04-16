#include "syscall.h"

#include "csr.h"
#include "current.h"
#include "exception.h"
#include "ipc.h"
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
static proc_t *syscall_pid_get(proc_t *proc, word_t args[8])
{
	args[0] = proc->pid;
	return proc;
}

/**
 * Get a virtual register of the current process.
 */
static proc_t *syscall_vreg_get(proc_t *proc, word_t args[8])
{
       switch (args[1]) {
       case VREG_TPC:
	       args[0] = proc->trap.tpc;
	       break;
       case VREG_TSP:
	       args[0] = proc->trap.tsp;
	       break;
       case VREG_ECAUSE:
	       args[0] = proc->trap.ecause;
	       break;
       case VREG_EVAL:
	       args[0] = proc->trap.eval;
	       break;
       case VREG_EPC:
	       args[0] = proc->trap.epc;
	       break;
       case VREG_ESP:
	       args[0] = proc->trap.esp;
	       break;
       default:
	       args[0] = 0;
	       break;
       }
       return proc;
}

/**
 * Set a virtual register of the current process.
 */
static proc_t *syscall_vreg_set(proc_t *proc, word_t args[8])
{
       switch (args[1]) {
       case VREG_TPC:
	       proc->trap.tpc = args[2];
	       break;
       case VREG_TSP:
	       proc->trap.tsp = args[2];
	       break;
       case VREG_ECAUSE:
	       proc->trap.ecause = args[2];
	       break;
       case VREG_EVAL:
	       proc->trap.eval = args[2];
	       break;
       case VREG_EPC:
	       proc->trap.epc = args[2];
	       break;
       case VREG_ESP:
	       proc->trap.esp = args[2];
	       break;
       default:
	       break;
       }
       return proc;
}

/**
 * Release the process, then call the scheduler.
 */
static proc_t *syscall_sync(proc_t *proc, word_t args[8])
{
	(void)args;
	proc->timeout = 0;
	proc_release(proc->pid);
	return NULL;
}

/**
 * Sleep until the specified timeout, then call the scheduler.
 */
static proc_t *syscall_sleep_until(proc_t *proc, word_t args[8])
{
       if (args[1] != 0) {
	       proc->timeout = args[1];
       }
       proc_release(proc->pid);
       return NULL;
}

/**
 * Get a memory capability.
 */
static proc_t *syscall_mem_introspect(proc_t *proc, word_t args[8])
{
	args[0] = mem_introspect(proc->pid, args[1], args[2], (mem_t *)&args[1]);
	return proc;
}

/**
 * Get a time slice capability.
 */
static proc_t *syscall_tsl_introspect(proc_t *proc, word_t args[8])
{
	args[0] = tsl_introspect(proc->pid, args[1], args[2], (tsl_t *)&args[1]);
	return proc;
}

/**
 * Get a monitor capability.
 */
static proc_t *syscall_mon_introspect(proc_t *proc, word_t args[8])
{
	args[0] = mon_introspect(proc->pid, args[1], args[2], (mon_t *)&args[1]);
	return proc;
}

/**
 * Get an IPC capability.
 */
static proc_t *syscall_ipc_introspect(proc_t *proc, word_t args[8])
{
	args[0] = ipc_introspect(proc->pid, args[1], args[2], (ipc_t *)&args[1]);
	return proc;
}

/**
 * Derive a memory capability.
 */
static proc_t *syscall_mem_derive(proc_t *proc, word_t args[8])
{
	args[0] = mem_derive(proc->pid, args[1], proc->pid, args[2], args[3], args[4], args[5]);
	return proc;
}

/**
 * Derive a time slice capability.
 */
static proc_t *syscall_tsl_derive(proc_t *proc, word_t args[8])
{
	args[0] = tsl_derive(proc->pid, args[1], proc->pid, args[2], args[3], args[4]);
	return proc;
}

/**
 * Derive a monitor capability.
 */
static proc_t *syscall_mon_derive(proc_t *proc, word_t args[8])
{
	args[0] = mon_derive(proc->pid, args[1], proc->pid, args[2]);
	return proc;
}

/**
 * Derive an IPC capability.
 */
static proc_t *syscall_ipc_derive(proc_t *proc, word_t args[8])
{
	args[0] = ipc_derive(proc->pid, args[1], proc->pid, args[2], args[3], args[4]);
	return proc;
}

/**
 * Revoke the children of a memory capability.
 */
static proc_t *syscall_mem_revoke(proc_t *proc, word_t args[8])
{
	args[0] = mem_revoke(proc->pid, args[1]);
	return proc;
}

/**
 * Revoke the children of a time slice capability.
 */
static proc_t *syscall_tsl_revoke(proc_t *proc, word_t args[8])
{
	args[0] = tsl_revoke(proc->pid, args[1]);
	return proc;
}

/**
 * Revoke the children of a monitor capability.
 */
static proc_t *syscall_mon_revoke(proc_t *proc, word_t args[8])
{
	args[0] = mon_revoke(proc->pid, args[1]);
	return proc;
}

/**
 * Revoke the children of an IPC capability.
 */
static proc_t *syscall_ipc_revoke(proc_t *proc, word_t args[8])
{
	args[0] = ipc_revoke(proc->pid, args[1]);
	return proc;
}

/**
 * Delete a memory capability.
 */
static proc_t *syscall_mem_delete(proc_t *proc, word_t args[8])
{
	args[0] = mem_delete(proc->pid, args[1]);
	return proc;
}

/**
 * Delete a time slice capability.
 */
static proc_t *syscall_tsl_delete(proc_t *proc, word_t args[8])
{
	args[0] = tsl_delete(proc->pid, args[1]);
	return proc;
}

/**
 * Delete a monitor capability.
 */
static proc_t *syscall_mon_delete(proc_t *proc, word_t args[8])
{
	args[0] = mon_delete(proc->pid, args[1]);
	return proc;
}

/**
 * Delete an IPC capability
 */
static proc_t *syscall_ipc_delete(proc_t *proc, word_t args[8])
{
	args[0] = ipc_delete(proc->pid, args[1]);
	return proc;
}

/**
 * Get a memory capability's PMP configuration.
 */
static proc_t *syscall_mem_pmp_get(proc_t *proc, word_t args[8])
{
	pmp_slot_t slot;
	mem_perm_t rwx;
	pmp_addr_t addr;
	args[0] = mem_pmp_get(proc->pid, args[1], &slot, &rwx, &addr);
	args[1] = slot;
	args[2] = rwx;
	args[3] = addr;
	return proc;
}

/**
 * Set a memory capability's PMP configuration.
 */
static proc_t *syscall_mem_pmp_set(proc_t *proc, word_t args[8])
{
	args[0] = mem_pmp_set(proc->pid, args[1], args[2], args[3], args[4]);
	return proc;
}

/**
 * Clear a memory capability's PMP configuration.
 */
static proc_t *syscall_mem_pmp_clear(proc_t *proc, word_t args[8])
{
	args[0] = mem_pmp_clear(proc->pid, args[1]);
	return proc;
}

/**
 * Enable or disable a time slice capability's minor frame.
 */
static proc_t *syscall_tsl_set(proc_t *proc, word_t args[8])
{
	args[0] = tsl_set(proc->pid, args[1], args[2]);
	return proc;
}

/**
 * Suspend the process that is being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_suspend(proc_t *proc, word_t args[8])
{
	args[0] = mon_suspend(proc->pid, args[1]);
	return proc;
}

/**
 * Resume the process that is being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_resume(proc_t *proc, word_t args[8])
{
	args[0] = mon_resume(proc->pid, args[1]);
	return proc;
}

/**
 * Yield execution time to the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_yield(proc_t *proc, word_t args[8])
{
	proc_t *next = proc;
	args[0] = mon_yield(proc->pid, args[1], &next);
	return next;
}

/**
 * Get a register value of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_reg_get(proc_t *proc, word_t args[8])
{
	word_t value;
	args[0] = mon_reg_get(proc->pid, args[1], args[2], &value);
	args[1] = value;
	return proc;
}

/**
 * Set a register of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_reg_set(proc_t *proc, word_t args[8])
{
	args[0] = mon_reg_set(proc->pid, args[1], args[2], args[3]);
	return proc;
}

/**
 * Get a virtual register value of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_vreg_get(proc_t *proc, word_t args[8])
{
	word_t value;
	args[0] = mon_vreg_get(proc->pid, args[1], args[2], &value);
	args[1] = value;
	return proc;
}

/**
 * Set a virtual register of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_vreg_set(proc_t *proc, word_t args[8])
{
	args[0] = mon_vreg_set(proc->pid, args[1], args[2], args[3]);
	return proc;
}

/**
 * Get a time slice capability configuration of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_tsl_introspect(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = tsl_introspect(target, args[2], args[3], (tsl_t *)&args[1]);
       }
       return proc;
}

/**
 * Get a memory capability configuration of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_mem_introspect(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = mem_introspect(target, args[2], args[3], (mem_t *)&args[1]);
       }
       return proc;
}

/**
 * Get a monitor capability configuration of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_mon_introspect(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = mon_introspect(target, args[2], args[3], (mon_t *)&args[1]);
       }
       return proc;
}

/**
 * Get an IPC capability configuration of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_ipc_introspect(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = ipc_introspect(target, args[2], args[3], (ipc_t *)&args[1]);
       }
       return proc;
}

/**
 * Grant a memory capability to the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_mem_grant(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = mem_transfer(proc->pid, args[2], target);
       }
       return proc;
}

/**
 * Grant a time slice capability to the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_tsl_grant(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = tsl_transfer(proc->pid, args[2], target);
       }
       return proc;
}

/**
 * Grant a monitor capability to the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_mon_grant(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = mon_transfer(proc->pid, args[2], target);
       }
       return proc;
}

/**
 * Grant an IPC capability to the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_ipc_grant(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = ipc_transfer(proc->pid, args[2], target);
       }
       return proc;
}

/**
 * Derive then grant a time slice capability to the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_tsl_derive(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = tsl_derive(proc->pid, args[2], target, args[3], args[4], args[5]);
       }
       return proc;
}

/**
 * Get a memory capability configuration of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_mem_derive(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = mem_derive(proc->pid, args[2], target, args[3], args[4], args[5], args[6]);
       }
       return proc;
}

/**
 * Get a monitor capability configuration of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_mon_derive(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = mon_derive(proc->pid, args[2], target, args[3]);
       }
       return proc;
}

/**
 * Get an IPC capability configuration of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_ipc_derive(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = ipc_derive(proc->pid, args[2], target, args[3], args[4], args[5]);
       }
       return proc;
}

/**
 * Get a memory capability PMP configuration of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_mem_pmp_get(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       pmp_slot_t slot;
	       mem_perm_t rwx;
	       pmp_addr_t addr;
	       args[0] = mem_pmp_get(target, args[2], &slot, &rwx, &addr);
	       args[1] = slot;
	       args[2] = rwx;
	       args[3] = addr;
       }
       return proc;
}

/**
 * Set a memory capability PMP configuration of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_mem_pmp_set(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = mem_pmp_set(target, args[2], args[3], args[4], args[5]);
       }
       return proc;
}

/**
 * Clear a memory capability PMP configuration of the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_mem_pmp_clear(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = mem_pmp_clear(target, args[2]);
       }
       return proc;
}

/**
 * Grant a time slice capability to the process being monitored by the specified monitor capability.
 */
static proc_t *syscall_mon_tsl_set(proc_t *proc, word_t args[8])
{
       pid_t target = mon_get_pid(proc->pid, args[1]);
       args[0] = ERR_INVALID_ACCESS;
       if (target != INVALID_PID) {
	       args[0] = tsl_set(target, args[2], args[3]);
       }
       return proc;
}

/**
 * Send an synchronous IPC message in a unidirectional IPC channel.
 */
static proc_t *syscall_ipc_send(proc_t *proc, word_t args[8])
{
	proc_t *next = proc;
	word_t data[2] = {args[2], args[3]};
	args[0] = ipc_send(proc->pid, args[1], data, args[4], args[5], &next);
	return next;
}

/**
 * Wait to receive an IPC message (synchronous, bidirectional/unidirectional IPC).
 */
static proc_t *syscall_ipc_recv(proc_t *proc, word_t args[8])
{
	proc_t *next = proc;
	args[0] = ipc_recv(proc->pid, args[1], &next, args[2]);
	return next;
}

/**
 * Make an IPC call (synchronous, bidirectional IPC channel) to an IPC server.
 */
static proc_t *syscall_ipc_call(proc_t *proc, word_t args[8])
{
	proc_t *next = proc;
	word_t data[2] = {args[2], args[3]};
	args[0] = ipc_call(proc->pid, args[1], data, args[4], args[5], &next);
	return next;
}

/**
 * Send a reply to an IPC call.
 */
static proc_t *syscall_ipc_reply(proc_t *proc, word_t args[8])
{
	proc_t *next = proc;
	word_t data[2] = {args[2], args[3]};
	args[0] = ipc_reply(proc->pid, args[1], data, args[4], args[5], &next);
	return next;
}

/**
 * Send a reply for an IPC call, then atomically wait to receive an IPC message.
 */
static proc_t *syscall_ipc_replyrecv(proc_t *proc, word_t args[8])
{
	proc_t *next = proc;
	word_t data[2] = {args[2], args[3]};
	args[0] = ipc_replyrecv(proc->pid, args[1], data, args[4], args[5], &next, args[6]);
	return next;
}

/**
 * Send an asynchronous IPC message in a unidirectional IPC channel.
 */
static proc_t *syscall_ipc_asend(proc_t *proc, word_t args[8])
{
	proc_t *next = proc;
	args[0] = ipc_asend(proc->pid, args[1], args[2], &next);
	return next;
}

/**
 * Read the message inbox of an asynchronous IPC channel.
 */
static proc_t *syscall_ipc_arecv(proc_t *proc, word_t args[8])
{
	proc_t *next = proc;
	word_t data;
	args[0] = ipc_arecv(proc->pid, args[1], &data);
	args[1] = data;
	return next;
}

static proc_t *syscall_invalid(proc_t *proc, word_t args[8])
{
	args[0] = ERR_INVALID_SYSCALL;
	return proc;
}

/**
 * Handler type for system calls.
 */
typedef proc_t *(*handler_t)(proc_t *proc, word_t args[8]);

/**
 * Handlers for individual system calls.
 */
handler_t handlers[] = {
	syscall_pid_get,
	syscall_vreg_get,
	syscall_vreg_set,
	syscall_sync,
	syscall_sleep_until,
	syscall_mem_introspect,
	syscall_tsl_introspect,
	syscall_mon_introspect,
	syscall_ipc_introspect,
	syscall_mem_derive,
	syscall_tsl_derive,
	syscall_mon_derive,
	syscall_ipc_derive,
	syscall_mem_revoke,
	syscall_tsl_revoke,
	syscall_mon_revoke,
	syscall_ipc_revoke,
	syscall_mem_delete,
	syscall_tsl_delete,
	syscall_mon_delete,
	syscall_ipc_delete,
	syscall_mem_pmp_get,
	syscall_mem_pmp_set,
	syscall_mem_pmp_clear,
	syscall_tsl_set,
	syscall_mon_suspend,
	syscall_mon_resume,
	syscall_mon_yield,
	syscall_mon_reg_get,
	syscall_mon_reg_set,
	syscall_mon_vreg_get,
	syscall_mon_vreg_set,
	syscall_mon_mem_introspect,
	syscall_mon_tsl_introspect,
	syscall_mon_mon_introspect,
	syscall_mon_ipc_introspect,
	syscall_mon_mem_grant,
	syscall_mon_tsl_grant,
	syscall_mon_mon_grant,
	syscall_mon_ipc_grant,
	syscall_mon_mem_derive,
	syscall_mon_tsl_derive,
	syscall_mon_mon_derive,
	syscall_mon_ipc_derive,
	syscall_mon_mem_pmp_get,
	syscall_mon_mem_pmp_set,
	syscall_mon_mem_pmp_clear,
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
proc_t* syscall_handler(proc_t *proc)
{
	// The system call number.
	word_t syscall_nr = proc->regs.a0;

	// Advance the program counter.
	proc->regs.pc += 4;

	// If system call number is invalid, make an exception.
	handler_t handler = (syscall_nr < ARRAY_SIZE(handlers)) ? handlers[syscall_nr] : syscall_invalid;
	return handler(proc, &current->regs.a0);
}

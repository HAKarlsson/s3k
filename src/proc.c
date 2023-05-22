/* See LICENSE file for copyright and license details. */
#include "proc.h"

#include "cap_operations.h"
#include "csr.h"
#include "kassert.h"
#include "ticket_lock.h"

static proc_t _processes[NUM_OF_PROCESSES];
extern unsigned char _payload[];

void proc_init(void)
{
	for (int i = 0; i < NUM_OF_PROCESSES; i++) {
		_processes[i].pid = i;
		_processes[i].state = PS_SUSPENDED;
	}
	_processes[0].state = PS_READY;
	_processes[0].regs[REG_PC] = (uint64_t)_payload;

	// We assume that the first capability gives us memory
	cap_pmp_set(0, 0);
}

proc_t *proc_get(uint64_t pid)
{
	kassert(pid < NUM_OF_PROCESSES);
	kassert(_processes[pid].pid == pid);
	return &_processes[pid];
}

bool proc_acquire(proc_t *proc)
{
	// Set the busy flag if expected state
	uint64_t expected = 0;
	uint64_t desired = PSF_BUSY;
	return __atomic_compare_exchange_n(
	    &proc->state, &expected, desired, false /* not weak */,
	    __ATOMIC_ACQUIRE /* succ */, __ATOMIC_RELAXED /* fail */);
}

bool proc_monitor_acquire(proc_t *proc)
{
	uint64_t expected = proc->state & PSF_SUSPEND;
	uint64_t desired = expected | PSF_BUSY;
	return __atomic_compare_exchange_n(
	    &proc->state, &expected, desired, false /* not weak */,
	    __ATOMIC_ACQUIRE /* succ */, __ATOMIC_RELAXED /* fail */);
}

void proc_release(proc_t *proc)
{
	// Unset all flags except suspend flag.
	__atomic_fetch_and(&proc->state, PSF_SUSPEND, __ATOMIC_RELEASE);
}

void proc_suspend(proc_t *proc)
{
	// Set the suspend flag
	uint64_t prev_state = __atomic_fetch_or(&proc->state, PSF_SUSPEND, __ATOMIC_ACQUIRE);

	// If the process was waiting, we also unset the waiting flag.
	if ((prev_state & 0xFF) == PSF_WAITING) {
		proc->state = PSF_SUSPEND;
		__atomic_thread_fence(__ATOMIC_ACQUIRE);
	}
}

void proc_resume(proc_t *proc)
{
	// Unset the suspend flag
	__atomic_fetch_and(&proc->state, ~PSF_SUSPEND, __ATOMIC_RELEASE);
}

bool proc_ipc_wait(proc_t *proc, uint64_t channel_id)
{
	// We expect that the process is running as usual.
	uint64_t expected = PSF_BUSY;
	// We set the
	uint64_t desired = (channel_id << 48) | PSF_WAITING;
	// This should fail only if the process should be suspended.
	return __atomic_compare_exchange_n(
	    &proc->state, &expected, desired, false /* not weak */,
	    __ATOMIC_SEQ_CST /* succ */, __ATOMIC_RELAXED /* fail */);
}

bool proc_ipc_acquire(proc_t *proc, uint64_t channel_id)
{
	// We expect that the process is waiting on channel_id.
	// Channel ID is set to the most significant bytes.
	uint64_t expected = (channel_id << 48) | PSF_WAITING;
	uint64_t desired = expected | PSF_BUSY;
	return __atomic_compare_exchange_n(
	    &proc->state, &expected, desired, false /* not weak */,
	    __ATOMIC_SEQ_CST /* succ */, __ATOMIC_RELAXED /* fail */);
}

void proc_pmp_set(proc_t *proc, uint64_t index, uint64_t addr, uint64_t rwx)
{
	proc->pmpcfg[i] = rwx | 0x18;
	proc->pmpaddr[i] = addr;
}

void proc_pmp_clear(proc_t *proc, uint64_t index)
{
	proc->pmpcfg[i] = 0;
}

bool proc_pmp_is_set(proc_t *proc, uint64_t index)
{
	return proc->pmpcfg[i] != 0;
}

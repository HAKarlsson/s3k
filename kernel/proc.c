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
		_processes[i].state = PSF_SUSPENDED;
		_processes[i].instrument_wcet = 0;
	}
	_processes[0].state = 0;
	_processes[0].regs[REG_PC] = (uint64_t)_payload;

	// The first capability is PMP for boot
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
	uint64_t expected = proc->state & PSF_SUSPENDED;
	uint64_t desired = expected | PSF_BUSY;
	return __atomic_compare_exchange_n(
	    &proc->state, &expected, desired, false /* not weak */,
	    __ATOMIC_ACQUIRE /* succ */, __ATOMIC_RELAXED /* fail */);
}

void proc_release(proc_t *proc)
{
	// Unset all flags except suspend flag.
	__atomic_fetch_and(&proc->state, ~PSF_BUSY, __ATOMIC_RELEASE);
}

void proc_suspend(proc_t *proc)
{
	// Set the suspend flag
	uint64_t prev_state = __atomic_fetch_or(&proc->state, PSF_SUSPENDED, __ATOMIC_ACQUIRE);

	// If the process was waiting, we also unset the waiting flag.
	if ((prev_state & 0xFF) == PSF_BLOCKED) {
		proc->state = PSF_SUSPENDED;
		__atomic_thread_fence(__ATOMIC_ACQUIRE);
	}
}

void proc_resume(proc_t *proc)
{
	// Unset the suspend flag
	__atomic_fetch_and(&proc->state, ~PSF_SUSPENDED, __ATOMIC_RELEASE);
}

bool proc_ipc_wait(proc_t *proc, uint64_t channel_id)
{
	// We expect that the process is running as usual.
	uint64_t expected = PSF_BUSY;
	// We set process to be blocked on ipc
	uint64_t desired = (channel_id << 16) | PSF_BLOCKED;
	// This should fail only if the process should be suspended.
	return __atomic_compare_exchange_n(
	    &proc->state, &expected, desired, false /* not weak */,
	    __ATOMIC_SEQ_CST /* succ */, __ATOMIC_RELAXED /* fail */);
}

bool proc_ipc_acquire(proc_t *proc, uint64_t channel_id)
{
	// We expect that the process is waiting on channel_id.
	// Channel ID is set to the most significant bytes.
	uint64_t expected = (channel_id << 16) | PSF_BLOCKED;
	uint64_t desired = expected | PSF_BUSY;
	return __atomic_compare_exchange_n(
	    &proc->state, &expected, desired, false /* not weak */,
	    __ATOMIC_SEQ_CST /* succ */, __ATOMIC_RELAXED /* fail */);
}

void proc_ipc_release(proc_t *proc)
{
	__atomic_fetch_and(&proc->state, PSF_SUSPENDED, __ATOMIC_RELEASE);
}

void proc_pmp_set(proc_t *proc, uint64_t index, uint64_t addr, uint64_t rwx)
{
	uint64_t mode = 0x18;
	proc->pmpcfg[index] = rwx | mode;
	proc->pmpaddr[index] = addr;
}

void proc_pmp_clear(proc_t *proc, uint64_t index)
{
	proc->pmpcfg[index] = 0;
}

bool proc_pmp_is_set(proc_t *proc, uint64_t index)
{
	return proc->pmpcfg[index] != 0;
}

void proc_pmp_load(proc_t *proc)
{
	csrw_pmpcfg0(*((uint64_t *)proc->pmpcfg));
	csrw_pmpaddr0(proc->pmpaddr[0]);
	csrw_pmpaddr1(proc->pmpaddr[1]);
	csrw_pmpaddr2(proc->pmpaddr[2]);
	csrw_pmpaddr3(proc->pmpaddr[3]);
	csrw_pmpaddr4(proc->pmpaddr[4]);
	csrw_pmpaddr5(proc->pmpaddr[5]);
	csrw_pmpaddr6(proc->pmpaddr[6]);
	csrw_pmpaddr7(proc->pmpaddr[7]);
}

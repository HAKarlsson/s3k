/* See LICENSE file for copyright and license details. */
#include "proc.h"

#include "cnode.h"
#include "csr.h"
#include "kassert.h"
#include "ticket_lock.h"

static proc_t _processes[PROC_COUNT];
extern unsigned char _payload[];

void proc_init(void)
{
	for (int i = 0; i < PROC_COUNT; i++) {
		_processes[i].pid = i;
		_processes[i].state = PS_SUSPENDED;
	}
	_processes[0].state = PS_READY;
	_processes[0].regs.pc = (uint64_t)_payload;
}

proc_t *proc_get(uint64_t pid)
{
	kassert(pid < PROC_COUNT);
	return &_processes[pid];
}

bool proc_acquire(proc_t *proc, uint64_t expected)
{
	kassert(!(expected & PSF_BUSY));
	// Set the busy flag if expected state
	uint64_t desired = expected | PSF_BUSY;
	return __atomic_compare_exchange_n(&proc->state, &expected, desired,
					   false /* not weak */,
					   __ATOMIC_ACQUIRE /* succ */,
					   __ATOMIC_RELAXED /* fail */);
}

void proc_release(proc_t *proc)
{
	// Unset all flags except suspend flag.
	__atomic_fetch_and(&proc->state, PSF_SUSPEND, __ATOMIC_RELEASE);
}

void proc_suspend(proc_t *proc)
{
	// Set the suspend flag
	uint64_t prev_state
	    = __atomic_fetch_or(&proc->state, PSF_SUSPEND, __ATOMIC_ACQUIRE);

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
	return __atomic_compare_exchange_n(&proc->state, &expected, desired,
					   false /* not weak */,
					   __ATOMIC_SEQ_CST /* succ */,
					   __ATOMIC_RELAXED /* fail */);
}

bool proc_ipc_acquire(proc_t *proc, uint64_t channel_id)
{
	// We expect that the process is waiting on channel_id.
	// Channel ID is set to the most significant bytes.
	uint64_t expected = (channel_id << 48) | PSF_WAITING;
	return proc_acquire(proc, expected);
}

void proc_pmp_load(const proc_t *proc)
{
	uint64_t pmpcfg0 = *(uint64_t *)proc->pmpcfg;
	uint64_t pmpaddr0 = proc->pmpaddr[0];
	uint64_t pmpaddr1 = proc->pmpaddr[1];
	uint64_t pmpaddr2 = proc->pmpaddr[2];
	uint64_t pmpaddr3 = proc->pmpaddr[3];
	uint64_t pmpaddr4 = proc->pmpaddr[4];
	uint64_t pmpaddr5 = proc->pmpaddr[5];
	uint64_t pmpaddr6 = proc->pmpaddr[6];
	uint64_t pmpaddr7 = proc->pmpaddr[7];
	csrw_pmpcfg0(pmpcfg0);
	csrw_pmpaddr0(pmpaddr0);
	csrw_pmpaddr1(pmpaddr1);
	csrw_pmpaddr2(pmpaddr2);
	csrw_pmpaddr3(pmpaddr3);
	csrw_pmpaddr4(pmpaddr4);
	csrw_pmpaddr5(pmpaddr5);
	csrw_pmpaddr6(pmpaddr6);
	csrw_pmpaddr7(pmpaddr7);
}

void proc_pmp_set(proc_t *proc, uint64_t i, uint64_t addr, uint64_t rwx)
{
	proc->pmpcfg[i] = addr | 0x18;
	proc->pmpaddr[i] = rwx;
}

void proc_pmp_clear(proc_t *proc, uint64_t i)
{
	proc->pmpcfg[i] = 0;
}

bool proc_pmp_is_set(proc_t *proc, uint64_t i)
{
	return proc->pmpcfg[i] != 0;
}

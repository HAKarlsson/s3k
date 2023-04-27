/* See LICENSE file for copyright and license details. */
#include "proc.h"

#include "cnode.h"
#include "csr.h"
#include "kassert.h"

static proc_t _processes[NPROC];
extern unsigned char _payload[];

void proc_init(void)
{
	for (int i = 0; i < NPROC; i++) {
		_processes[i].pid = i;
		_processes[i].state = PS_SUSPENDED;
	}
	_processes[0].state = PS_READY;
	_processes[0].regs[REG_PC] = (uint64_t)_payload;
}

proc_t *proc_get(uint64_t pid)
{
	kassert(pid < NPROC);
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

void proc_load_pmp(const proc_t *proc)
{
	uint8_t *pmp = (uint8_t *)&proc->regs[REG_PMP];
	uint64_t pmpcfg = 0;
	cap_t cap;

	cap = cnode_cap(cnode_idx(proc->pid, pmp[0]));
	if (cap.type == CAPTY_PMP) {
		pmpcfg |= (uint64_t)cap.pmp.cfg;
		csrw_pmpaddr0((uint64_t)cap.pmp.addr);
	}
	cap = cnode_cap(cnode_idx(proc->pid, pmp[1]));
	if (cap.type == CAPTY_PMP) {
		pmpcfg |= (uint64_t)cap.pmp.cfg << 8;
		csrw_pmpaddr1((uint64_t)cap.pmp.addr);
	}
	cap = cnode_cap(cnode_idx(proc->pid, pmp[2]));
	if (cap.type == CAPTY_PMP) {
		pmpcfg |= (uint64_t)cap.pmp.cfg << 16;
		csrw_pmpaddr2((uint64_t)cap.pmp.addr);
	}
	cap = cnode_cap(cnode_idx(proc->pid, pmp[3]));
	if (cap.type == CAPTY_PMP) {
		pmpcfg |= (uint64_t)cap.pmp.cfg << 24;
		csrw_pmpaddr3((uint64_t)cap.pmp.addr);
	}
	cap = cnode_cap(cnode_idx(proc->pid, pmp[4]));
	if (cap.type == CAPTY_PMP) {
		pmpcfg |= (uint64_t)cap.pmp.cfg << 32;
		csrw_pmpaddr4((uint64_t)cap.pmp.addr);
	}
	cap = cnode_cap(cnode_idx(proc->pid, pmp[5]));
	if (cap.type == CAPTY_PMP) {
		pmpcfg |= (uint64_t)cap.pmp.cfg << 40;
		csrw_pmpaddr5((uint64_t)cap.pmp.addr);
	}
	cap = cnode_cap(cnode_idx(proc->pid, pmp[6]));
	if (cap.type == CAPTY_PMP) {
		pmpcfg |= (uint64_t)cap.pmp.cfg << 48;
		csrw_pmpaddr6((uint64_t)cap.pmp.addr);
	}
	cap = cnode_cap(cnode_idx(proc->pid, pmp[7]));
	if (cap.type == CAPTY_PMP) {
		pmpcfg |= (uint64_t)cap.pmp.cfg << 56;
		csrw_pmpaddr7((uint64_t)cap.pmp.addr);
	}

	csrw_pmpcfg0(pmpcfg);
}

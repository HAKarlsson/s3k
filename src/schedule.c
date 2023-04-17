/* See LICENSE file for copyright and license details. */

#include "schedule.h"

#include "consts.h"
#include "csr.h"
#include "platform.h"
#include "proc.h"
#include "timer.h"
#include "trap.h"
#include "wfi.h"

#define NONE_PID 0xFF

static volatile struct sched_entry _schedule[NHART][NSLICE];

struct sched_entry schedule_get(uint64_t hartid, size_t i)
{
	return _schedule[hartid][i];
}

void schedule_update(uint64_t hartid, uint64_t pid, uint64_t begin,
		     uint64_t end)
{
	for (uint64_t i = begin; i < end; i++)
		_schedule[hartid][i] = (struct sched_entry){ pid, end - i };
}

void schedule_delete(uint64_t hartid, uint64_t begin, uint64_t end)
{
	schedule_update(hartid, NONE_PID, begin, end);
}

struct proc *schedule_next()
{
	uint64_t hartid;
	uint64_t quantum, start_time, end_time;
	struct proc *proc;
	struct sched_entry entry;

	// Get the hart ID.
	hartid = csrr_mhartid();

retry:
	// Get the quantum.
	quantum = (time_get() + NSLACK) / NTICK;

	// Get the scheduled process
	entry = schedule_get(hartid, quantum % NSLICE);
	if (entry.pid == NONE_PID)
		goto retry;

	// Get the process
	proc = proc_get(entry.pid);

	// Calculate start and end time.
	start_time = quantum * NTICK;
	end_time = start_time + entry.len * NTICK - NSLACK;

	// Check if process is sleeping.
	if (proc->sleep > end_time)
		goto retry;

	// Set start_time to sleep if sleep is larger.
	if (proc->sleep > start_time)
		start_time = proc->sleep;

	// Setup the PMP.
	proc_load_pmp(proc);

	// Temporary fix, QEMU does not allow this to be zero.
	if (!csrr_pmpcfg0())
		goto retry;

	// Acquire the process.
	if (!proc_acquire(proc, PS_READY))
		goto retry;

	// Wait until start_time
	timeout_set(hartid, start_time);
	while (!(csrr_mip() & (1 << 7)))
		wfi();

	// Set timeout
	timeout_set(hartid, end_time);

	return proc;
}

struct proc *schedule_yield(struct proc *proc)
{
	// Release currently held process.
	proc_release(proc);
	// Schedule the next process.
	return schedule_next();
}

void schedule_init(void)
{
	for (int i = 0; i < NHART; i++)
		schedule_update(i, 0, 0, NSLICE);
}

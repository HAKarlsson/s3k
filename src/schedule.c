/* See LICENSE file for copyright and license details. */

#include "schedule.h"

#include "consts.h"
#include "csr.h"
#include "current.h"
#include "platform.h"
#include "proc.h"
#include "timer.h"
#include "trap.h"
#include "wfi.h"

#define NONE_PID 0xFF

static volatile struct sched_entry _schedule[NHART][NSLICE];

struct sched_entry schedule_get(uint64_t hartid, size_t quantum)
{
	struct sched_entry entry = _schedule[hartid][quantum];
	return entry;
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

void schedule_next()
{
	uint64_t hartid;
	uint64_t quantum, start_time, end_time;
	struct sched_entry entry;

	// Get the hart ID.
	hartid = csrr_mhartid();

retry:
	// Get the quantum.
	quantum = (time_get() + NSLACK) / NTICK;

	// Get the scheduled process
	entry = schedule_get(hartid, quantum % NSLICE);

	// If invalid, go back
	if (entry.pid == NONE_PID)
		goto retry;

	// Get the process
	current = proc_get(entry.pid);

	// Calculate start and end time.
	start_time = quantum * NTICK;
	end_time = start_time + entry.len * NTICK - NSLACK;

	// Check if process is sleeping.
	if (current->sleep > end_time)
		goto retry;

	// Set start_time to sleep if sleep is larger.
	if (current->sleep > start_time)
		start_time = current->sleep;

	// Setup the PMP.
	proc_load_pmp(current);

	// Temporary fix, QEMU does not allow this to be zero.
	if (!csrr_pmpcfg0())
		goto retry;

	// Acquire the process.
	if (!proc_acquire(current, PS_READY))
		goto retry;

	// Wait until start_time
	timeout_set(hartid, start_time);
	while (!(csrr_mip() & (1 << 7)))
		wfi();

	// Set timeout
	timeout_set(hartid, end_time);
}

void schedule_yield(void)
{
	// Release currently held process.
	proc_release(current);
	// Schedule the next process.
	schedule_next();
}

void schedule_init(void)
{
	for (int i = MIN_HARTID; i <= MAX_HARTID; i++)
		schedule_update(i, 0, 0, NSLICE);
}

/* See LICENSE file for copyright and license details. */

#include "schedule.h"

#include "csr.h"
#include "current.h"
#include "proc.h"
#include "time.h"
#include "wfi.h"

static uint16_t _frames[NUM_OF_HARTS][NUM_OF_FRAMES];

proc_t *schedule_get(uint64_t hartid, uint64_t current_time, uint64_t *start_time, uint64_t *end_time)
{
	// Calculate the current frame
	uint64_t start_frame = ((current_time + SLACK_LENGTH) / FRAME_LENGTH) % NUM_OF_FRAMES;

	// Get the frame information
	uint16_t frame_info = __atomic_load_n(&_frames[hartid - MIN_HARTID][start_frame], __ATOMIC_SEQ_CST);

	uint64_t pid = frame_info >> 8;
	uint64_t end = frame_info & 0xFF;

	if (end == 0) // Frame deleted.
		return NULL;

	proc_t *proc = proc_get(pid);
	uint64_t length = end - start_frame;

	// Calculate start and end times of frame.
	*start_time = ((current_time + SLACK_LENGTH) / FRAME_LENGTH) * FRAME_LENGTH;
	*end_time = *start_time + length * FRAME_LENGTH;

	if (proc->sleep > *end_time) // Process sleeping.
		return NULL;

	if (proc->sleep > *start_time) // Process start delay.
		*start_time = proc->sleep;

	return proc;
}

void schedule_init(void)
{
	uint64_t pid = 0;
	uint64_t begin = 0;
	uint64_t end = NUM_OF_FRAMES;
	for (uint64_t hartid = MIN_HARTID; hartid <= MAX_HARTID; hartid++)
		schedule_update(pid, end, hartid, begin, end);
}

void schedule_update(uint64_t pid, uint64_t end, uint64_t hartid, uint64_t from, uint64_t to)
{
	uint16_t fi = pid << 8 | end;
	uint16_t *p = &_frames[hartid - MIN_HARTID][from];
	uint16_t *ep = &_frames[hartid - MIN_HARTID][to];
	while (p != ep)
		*p++ = fi;
}

void schedule_delete(uint64_t hartid, uint64_t from, uint64_t to)
{
	schedule_update(0, 0, hartid, from, to);
}

void schedule_yield(void)
{
	proc_release(current);
	schedule();
}

void schedule(void)
{
	uint64_t hartid, current_time, start_time, end_time;

	// Get the hart ID.
	hartid = csrr_mhartid();

retry: // Get the quantum.
	current_time = time_get();

	// Get the scheduled process
	current = schedule_get(hartid, current_time, &start_time, &end_time);

	// If invalid, go back
	if (!current)
		goto retry;

	// Acquire the process.
	if (!proc_acquire(current))
		goto retry;

	if (current->sleep > end_time) {
		proc_release(current);
		goto retry;
	}

	if (current->sleep > start_time)
		start_time = current->sleep;

	current->start_time = start_time;
	current->end_time = end_time;

	// Set start time.
	timeout_set(hartid, start_time);

	while (!(csrr_mip() & (1 << 7)))
		wfi();

	// Set end time.
	timeout_set(hartid, end_time);
}

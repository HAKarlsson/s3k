/* See LICENSE file for copyright and license details. */

#include "schedule.h"

#include "csr.h"
#include "current.h"
#include "proc.h"
#include "time.h"
#include "trap.h"
#include "wfi.h"

typedef struct frame_info {
	uint8_t pid;
	uint8_t end;
} frame_info_t;

static frame_info_t _frames[NUM_OF_FRAMES][NUM_OF_HARTS];
static uint64_t _timestamp;

/**
 * We have three priority rules for assigning process to a hart:
 * 1. A hart currently running the process has the highest priority.
 * 2. A hart with the longest scheduling for the process have higher priority.
 * 3. A hart with the lowest hart ID has the higher priority.
 * The rules are enforced in above order.
 *
 * This function enforces rules 2 and 3. The first rule is enforced using a
 * lock on the process. If the process is running on another hart, that
 * hart will have a lock on that process and continue running.
 */
bool _has_priority(frame_info_t frame_info[NUM_OF_HARTS], uint64_t hartid)
{
	if (frame_info[hartid].end == 0)
		return false;
	// Check harts with lower ID.
	// If their scheduling is equal or longer, then they have priority.
	for (uint64_t i = 0; i < hartid; i++) {
		if (frame_info[i].pid != frame_info[hartid].pid)
			continue;
		if (frame_info[i].end >= frame_info[hartid].end)
			return false;
	}
	// Check harts with higher ID.
	// If their scheduling is longer, then they have priority.
	for (uint64_t i = hartid + 1; i < NUM_OF_HARTS; i++) {
		if (frame_info[i].pid != frame_info[hartid].pid)
			continue;
		if (frame_info[i].end > frame_info[hartid].end)
			return false;
	}
	return true;
}

void _set_timestamp(uint64_t timestamp)
{
	__atomic_store_n(&_timestamp, timestamp, __ATOMIC_SEQ_CST);
}

uint64_t _get_timestamp(void)
{
	return __atomic_load_n(&_timestamp, __ATOMIC_SEQ_CST);
}

uint64_t _get_quantum(uint64_t time)
{
	return ((time + SLACK_LENGTH) / FRAME_LENGTH) % NUM_OF_FRAMES;
}

uint64_t _get_frame_info(frame_info_t frame_info[NUM_OF_HARTS], uint64_t frame)
{
	for (uint64_t i = 0; i < NUM_OF_HARTS; i++)
		frame_info[i] = _frames[frame][i];
	return _get_timestamp();
}

proc_t *schedule_get(uint64_t hartid, uint64_t current_time, uint64_t *start_time, uint64_t *end_time)
{
	// Calculate the current frame
	uint64_t start_frame = _get_quantum(current_time);
	;

	frame_info_t frame_info[NUM_OF_HARTS];

	// Get frame info
	uint64_t timestamp = _get_frame_info(frame_info, start_frame);

	// Check if frame info is outdated.
	if (current_time < timestamp)
		return NULL;

	// Check if hart has priority on the process.
	if (!_has_priority(frame_info, hartid))
		return NULL;

	// Get the frame information
	uint64_t pid = frame_info[hartid].pid;
	uint64_t end = frame_info[hartid].end;

	if (end == 0) // Frame deleted.
		return NULL;

	proc_t *proc = proc_get(pid);
	uint64_t length = end - start_frame;

	// Calculate start and end times of frame.
	*start_time = ((current_time + SLACK_LENGTH) / FRAME_LENGTH) * FRAME_LENGTH;
	*end_time = *start_time + length * FRAME_LENGTH;

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
	for (uint64_t i = from; i < to; ++i) {
		_frames[i][hartid - MIN_HARTID] = (frame_info_t){ .pid = pid, .end = end };
	}
	__atomic_thread_fence(__ATOMIC_SEQ_CST);
	_timestamp = time_get();
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

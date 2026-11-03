#include "sched.h"

#include "csr.h"
#include "current.h"
#include "lock.h"
#include "preempt.h"
#include "rtc.h"

extern void temporal_fence(void);

typedef struct frame {
	uint16_t pid;
	uint16_t length;
} frame_t;

static frame_t schedule[_NUM_HARTS][MAX_TIME_SLOT];

void sched_init(void)
{
	for (hart_t hart = 0; hart < NUM_HARTS; ++hart) {
		for (time_slot_t slot = 0; slot < MAX_TIME_SLOT; ++slot) {
			schedule[hart][slot] = (frame_t){0};
		}
		schedule[hart][0] = (frame_t){
			.pid = hart == 0 ? 1 : INVALID_PID,
			.length = MAX_TIME_SLOT,
		};
	}
	rtc_set_time(0);
}

void sched_reclaim(hart_t hart, pid_t pid, time_slot_t begin, time_slot_t end)
{
	if (begin == end) {
		return;
	}
	schedule[hart][begin] = (frame_t){.pid = pid, .length = end - begin};
}

void sched_clear(hart_t hart, time_slot_t begin)
{
	schedule[hart][begin] = (frame_t){0};
}

void sched_split(hart_t hart, pid_t pid, time_slot_t begin, time_slot_t middle, time_slot_t end)
{
	schedule[hart][begin].length = middle - begin;
	schedule[hart][middle] = (frame_t){.pid = pid, .length = end - middle};
}

void sched_set_pid(hart_t hart, pid_t pid, time_slot_t begin)
{
	schedule[hart][begin].pid = pid;
}

static proc_result_t sched_select(hart_t hart, uint64_t *deadline)
{
	lock_acquire();
	uint64_t now = rtc_get_time();
	uint64_t slot = now / TIME_SLOT_TICKS;
	frame_t frame = schedule[hart][slot % MAX_TIME_SLOT];
	*deadline = (slot + 1) * TIME_SLOT_TICKS;
	proc_result_t result = 0;

	if (frame.length != 0 && frame.pid != INVALID_PID) {
		proc_t *proc = proc_get(frame.pid);
		if (proc->timeout <= now && proc_acquire(frame.pid)) {
			*deadline = (slot + frame.length) * TIME_SLOT_TICKS;
			proc->timeout = *deadline;
			result = proc_reload_result(proc);
		}
	}
	lock_release();
	return result;
}

proc_result_t sched(void)
{
	hart_t hart = csrr_mhartid();
	uint64_t deadline;
	proc_result_t result = sched_select(hart, &deadline);
	rtc_set_timeout(hart, deadline);
	return result;
}

proc_result_t sched_yield(void)
{
	lock_acquire();
	proc_release(current->pid);
	lock_release();
	return 0;
}

void sched_wait(void)
{
	temporal_fence();
	hart_t hart = csrr_mhartid();
	uint64_t deadline = rtc_get_timeout(hart);
	// MTIP can remain set briefly after mtimecmp is advanced.
	while (!(csrr_mip() & CSR_MIP_MTIP) || rtc_get_time() < deadline) {
		__asm__ volatile("wfi");
	}
}

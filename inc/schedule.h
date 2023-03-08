/**
 * @file sched.h
 * @brief Scheduler.
 * @copyright MIT License
 * @author Henrik Karlsson (henrik10@kth.se)
 * @bug QEMU mret does not work properly if all pmp registers are 0, so we have
 * a temporary fix in sched_next.
 */
#ifndef __SCHED_H__
#define __SCHED_H__

#include "common.h"
#include "proc.h"

#include <stdint.h>

struct sched_entry {
	uint8_t pid;
	uint8_t len;
};

struct sched_entry schedule_get(uint64_t hartid, size_t i);

/// Yield the current time slice.
void schedule_yield(struct proc *proc);
/// Find the next process to schedule.
void schedule_next(void);
/// Initialize scheduler.
void schedule_init(void);

/// Delete scheduling at hartid, begin-end.
void schedule_delete(uint64_t hartid, uint64_t begin, uint64_t end);
/// Let pid run on hartid, begin-end.
void schedule_update(uint64_t hartid, uint64_t pid, uint64_t begin,
		     uint64_t end);

#endif /* __SCHED_H__ */

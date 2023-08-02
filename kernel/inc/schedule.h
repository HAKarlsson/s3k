#pragma once
/**
 * @file sched.h
 * @brief Scheduler.
 * @copyright MIT License
 * @author Henrik Karlsson (henrik10@kth.se)
 * @bug QEMU mret does not work properly if all pmp registers are 0, so we have
 * a temporary fix in sched_next.
 */

#include "macro.h"
#include "proc.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Get the scheduling entry.
 *
 * Get the schedule entry of the hartid, position i.
 *
 * @param hartid The hartid for of the schedule entry.
 * @param i Index of the schedule entry.
 * @return Schedule entry (process ID and length of time slice).
 */
proc_t *schedule_get(uint64_t hartid, uint64_t curren_time, uint64_t *start_time, uint64_t *end_time);

/**
 * @brief Initialize the scheduler.
 *
 * This function initializes the scheduler, which is responsible for managing
 * the execution of processes on the system. It sets up the necessary data
 * structures and configurations to support scheduling.
 */
void schedule_init(void);

/**
 * @brief Find the next process to schedule.
 *
 * This function finds the next process to schedule based on the current
 * state of the system.
 */
void schedule_yield(void);
void schedule(void);

/// Delete scheduling at hartid, begin-end.
void schedule_delete(uint64_t hartid, uint64_t from, uint64_t to);

/// Let pid run on hartid, begin-end.
void schedule_update(uint64_t pid, uint64_t end, uint64_t hartid, uint64_t from, uint64_t to);

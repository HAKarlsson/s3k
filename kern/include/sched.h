#pragma once

#include "proc.h"
#include "types.h"

void sched_init(void);

/**
 * @brief Reclaims a range of scheduling slots for a specific process.
 *
 * Replaces the range with one frame after the caller has cleared obsolete
 * child entries. The caller holds the kernel lock.
 *
 * @param hart The hardware thread ID (hart) to reclaim slots for.
 * @param pid The process ID to assign the slots to.
 * @param begin The starting index of the slot range.
 * @param end The ending index of the slot range.
 */
void sched_reclaim(hart_t hart, pid_t pid, time_slot_t begin, time_slot_t end);

/**
 * Clears a revoked frame start while holding the kernel lock.
 */
void sched_clear(hart_t hart, time_slot_t begin);

/**
 * @brief Splits a scheduling slot into two parts.
 *
 * Divides a slot into two segments, assigning the first segment to the
 * original process and creating a new slot for the second segment.
 * The caller holds the kernel lock.
 *
 * @param hart The hardware thread ID (hart) to split the slot for.
 * @param pid The process ID of the slot to split.
 * @param begin The starting index of the slot.
 * @param mid The index where the slot is split.
 * @param end The ending index of the slot.
 */
void sched_split(hart_t hart, pid_t pid, time_slot_t begin, time_slot_t mid, time_slot_t end);

/**
 * @brief Sets the process ID for a specific scheduling slot.
 *
 * Updates the process ID for the specified slot in the scheduling table.
 * May set pid to INVALID_PID to disable the slot.
 *
 * @param hart The hardware thread ID (hart) to set the slot for.
 * @param pid The process ID to assign to the slot.
 * @param begin The index of the slot to update.
 */
void sched_set_pid(hart_t hart, pid_t pid, time_slot_t begin);

/**
 * Looks up the current frame start once and programs the local timer.
 * Returns zero or the negative address of an acquired PCB to reload PMP.
 */
proc_result_t sched(void);

/**
 * Releases the interrupted process under the kernel lock and returns zero.
 */
proc_result_t sched_yield(void);

/**
 * Performs the temporal fence and waits for the programmed timer deadline.
 */
void sched_wait(void);

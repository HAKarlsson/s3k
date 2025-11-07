#include "tsl.h"

#include "macro.h"
#include "preempt.h"
#include "proc.h"
#include "sched.h"

/**
 * Validates the arguments for accessing a time slice capability.
 */
bool tsl_valid_access(tsl_table_t *tt, pid_t owner, index_t i)
{
	return i < tt->size && tt->entries[i].owner == owner;
}

/**
 * Checks if a time slice capability can be derived from another.
 */
static bool _derivable(tsl_t *parent, fuel_t csize, time_slot_t size)
{
	return parent->cfree > csize && size <= parent->free && csize > 0 && size > 0;
}

/**
 * Transfers a time slice capability from one process to another.
 */
int tsl_transfer(tsl_table_t *tt, pid_t owner, index_t i, pid_t new_owner)
{
	if (UNLIKELY(!tsl_valid_access(tt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Update the owner of the capability.
	tt->entries[i].owner = new_owner;

	// Update the scheduler if the capability is enabled.
	if (tt->entries[i].free > 0) {
		sched_set_pid(tt->entries[i].hart, tt->entries[i].enabled ? new_owner : INVALID_PID, tt->entries[i].base);
	}

	return ERR_SUCCESS;
}

/**
 * Retrieves a time slice capability from the time table.
 */
int tsl_get(tsl_table_t *tt, pid_t owner, index_t i, tsl_t *time_cap)
{
	if (UNLIKELY(!tsl_valid_access(tt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Copy the time slice capability to the output parameter.
	*time_cap = tt->entries[i];

	return ERR_SUCCESS;
}

/**
 * Derives a new time slice capability from an existing one.
 */
int tsl_derive(tsl_table_t *tt, pid_t owner, index_t i, pid_t target, fuel_t csize, bool enable, time_slot_t size)
{
	if (UNLIKELY(!tsl_valid_access(tt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	if (UNLIKELY(!_derivable(&tt->entries[i], csize, size))) {
		return ERR_INVALID_ARGUMENT;
	}

	// Update the parent capability by reducing its cfree and adjusting its allocation.
	tt->entries[i].cfree -= csize;
	tt->entries[i].free -= size;

	// Calculate the index for the derived capability.
	index_t j = i + tt->entries[i].cfree;

	// Calculate the start of the new time slice capability
	time_slot_t base = tt->entries[i].base + tt->entries[i].free;

	// Create the new child capability with the specified parameters.
	tt->entries[j] = (tsl_t){
		.owner = target,
		.cfree = csize,
		.csize = csize,
		.hart = tt->entries[i].hart,
		.enabled = enable,
		.base = base,
		.size = size,
		.free = size,
	};

	// Update the scheduler with the new capability.
	pid_t sched_pid = enable ? target : INVALID_PID;
	sched_split(tt->entries[i].hart, sched_pid, tt->entries[i].base, base, base + size);

	// Return the index of the new capability.
	return j;
}

/**
 * Revokes the children of a time slice capability.
 */
int tsl_revoke(tsl_table_t *tt, pid_t owner, index_t i)
{
	if (UNLIKELY(!tsl_valid_access(tt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Reclaim cfree and invalidate children.
	while (tt->entries[i].cfree < tt->entries[i].csize) {
		index_t j = i + tt->entries[i].cfree;

		// Reclaim cfree and allocation.
		tt->entries[i].cfree += tt->entries[j].cfree;
		tt->entries[i].free += tt->entries[j].free;

		// Invalidate the child capability.
		tt->entries[j].owner = INVALID_PID;

		if (UNLIKELY(preempt()))
			break;
	}

	// Reclaim allocated time slots in the scheduler.
	pid_t pid = tt->entries[i].enabled ? owner : INVALID_PID;
	sched_reclaim(tt->entries[i].hart, pid, tt->entries[i].base, tt->entries[i].base + tt->entries[i].free);

	// Return the remaining cfree to be revoked.
	// Is 0 if all children are revoked.
	return tt->entries[i].csize - tt->entries[i].cfree;
}

/**
 * Deletes a time slice capability
 */
int tsl_delete(tsl_table_t *tt, pid_t owner, index_t i)
{
	if (UNLIKELY(!tsl_valid_access(tt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Invalidates the capability.
	tt->entries[i].owner = INVALID_PID;

	// Deletes the minor frame in the scheduler.
	if (tt->entries[i].free > 0) {
		sched_set_pid(tt->entries[i].hart, INVALID_PID, tt->entries[i].base);
	}

	return ERR_SUCCESS;
}

/**
 * Enables or disables a time slice capability.
 */
int tsl_set(tsl_table_t *tt, pid_t owner, index_t i, bool enable)
{
	if (UNLIKELY(!tsl_valid_access(tt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Enable or disable the minor frame in the scheduler.
	if (tt->entries[i].free > 0) {
		pid_t sched_pid = enable ? owner : INVALID_PID;
		sched_set_pid(tt->entries[i].hart, sched_pid, tt->entries[i].base);
	}
	// Make the time slice capability enabled or disabled.
	tt->entries[i].enabled = enable;

	return ERR_SUCCESS;
}

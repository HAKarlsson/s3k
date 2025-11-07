#include "mem.h"

#include "macro.h"
#include "pmp.h"
#include "preempt.h"
#include "proc.h"

/**
 * Check if a memory access is valid.
 */
bool mem_valid_access(mem_table_t *mt, pid_t owner, index_t i)
{
	return (i < mt->size) && (mt->entries[i].owner == owner);
}

/**
 * Check if the read-write-execute permissions are valid.
 */
static bool _valid_rwx(word_t rwx)
{
	return ((rwx & MEM_PERM_R) == MEM_PERM_R) || (rwx == MEM_PERM_NONE);
}

/**
 * Check if a memory capability can be derived.
 */
static bool _derivable(mem_t *parent, fuel_t csize, word_t rwx, word_t base, word_t size)
{
	return (base + size > base) && (parent->cfree > csize) && (parent->base <= base)
	       && (base + size <= parent->base + parent->size) && ((parent->rwx & rwx) == rwx) && (csize > 0)
	       && _valid_rwx(rwx);
}

/**
 * Check if the PMP arguments are valid.
 */
static bool _valid_pmp_args(mem_t *cap, word_t slot, word_t rwx, word_t addr)
{
	word_t base = pmp_napot_decode_base(addr);
	word_t size = pmp_napot_decode_size(addr);

	return (slot > 0) && (cap->slot == 0) && (cap->base <= base) && (base + size <= cap->base + cap->size)
	       && ((rwx & cap->rwx) == rwx) && _valid_rwx(rwx);
}

/**
 * Transfer a memory capability to a new owner.
 */
int mem_transfer(mem_table_t *mt, pid_t owner, index_t i, pid_t new_owner)
{
	if (UNLIKELY(!mem_valid_access(mt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// If PMP config is set, clear it.
	if (mt->entries[i].slot != 0) {
		proc_pmp_clear(owner, mt->entries[i].slot);
		mt->entries[i].slot = 0; // Clear the PMP slot.
	}

	// Set the new owner.
	mt->entries[i].owner = new_owner;

	return ERR_SUCCESS;
}

/**
 * Get a memory capability.
 */
int mem_get(mem_table_t *mt, pid_t owner, index_t i, mem_t *cap)
{
	if (UNLIKELY(!mem_valid_access(mt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	*cap = mt->entries[i];
	return ERR_SUCCESS;
}

/**
 * Derive a memory capability.
 */
int mem_derive(mem_table_t *mt, pid_t owner, index_t i, pid_t target, fuel_t cfree, mem_perm_t rwx, mem_addr_t base, mem_addr_t size)
{
	if (UNLIKELY(!mem_valid_access(mt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Check if the memory capability can be derived.
	if (UNLIKELY(!_derivable(&mt->entries[i], cfree, rwx, base, size))) {
		return ERR_INVALID_ARGUMENT;
	}

	// Update the current memory capability.
	mem_table[i].cfree -= cfree;

	// Calculate the index of the new memory capability.
	word_t j = i + mt->entries[i].cfree;

	// Create the new memory capability.
	mt->entries[j] = (mem_t){
		.owner = target,
		.cfree = cfree,
		.csize = cfree,
		.slot = 0,
		.rwx = rwx,
		.base = base,
		.size = size,
	};

	// Return the index of the new memory capability.
	return j;
}

/**
 * Revokes a memory capability and its children.
 */
int mem_revoke(mem_table_t *mt, pid_t owner, index_t i)
{
	if (UNLIKELY(!mem_valid_access(mt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Revoke the child capabilities.
	while (mt->entries[i].cfree < mt->entries[i].csize) {
		// Get the next child capability index.
		index_t j = i + mt->entries[i].cfree;

		// Clear the PMP slot if it is set.
		if (mt->entries[j].slot != 0) {
			proc_pmp_clear(mt->entries[j].owner, mt->entries[j].slot - 1);
		}
		// Invalidate the child capability.
		mt->entries[j].owner = INVALID_PID;

		// Reclaim the child's capability mt.
		mt->entries[i].cfree += mt->entries[j].cfree;

		if (UNLIKELY(preempt()))
			break;
	}

	// Return the number of unrevoked capabilities.
	return mt->entries[i].csize - mt->entries[i].cfree;
}

/**
 * Deletes a memory capability by invalidating its process ID.
 */
int mem_delete(mem_table_t *mt, pid_t owner, index_t i)
{
	if (UNLIKELY(!mem_valid_access(mt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Clear the PMP slot if it is set.
	if (mt->entries[i].slot != 0) {
		proc_pmp_clear(owner, mt->entries[i].slot - 1);
		mt->entries[i].slot = 0;
	}

	// Invalidate the capability.
	mt->entries[i].owner = INVALID_PID;

	return ERR_SUCCESS;
}

/**
 * Enables a memory capability by setting the PMP slot.
 */
int mem_pmp_set(mem_table_t *mt, pid_t owner, index_t i, pmp_slot_t slot, word_t rwx, word_t addr)
{
	if (UNLIKELY(!mem_valid_access(mt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Validate PMP arguments.
	if (UNLIKELY(!_valid_pmp_args(&mt->entries[i], slot, rwx, addr))) {
		return ERR_INVALID_ARGUMENT;
	}

	// Check if the slot is already in use.
	if (!proc_pmp_set(owner, slot - 1, rwx, addr)) {
		return ERR_SLOT_IN_USE;
	}
	mt->entries[i].slot = slot;

	return ERR_SUCCESS;
}

/**
 * Retrieves the PMP configuration for a memory capability.
 */
int mem_pmp_get(mem_table_t *mt, pid_t owner, index_t i, pmp_slot_t *slot, mem_perm_t *rwx, pmp_addr_t *addr)
{
	if (UNLIKELY(!mem_valid_access(mt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// If no PMP slot is set.
	if (mt->entries[i].slot == 0) {
		*slot = 0;
		*rwx = 0;
		*addr = 0;
		return ERR_SUCCESS;
	}

	// Retrieve the PMP configuration.
	*slot = mt->entries[i].slot;
	proc_pmp_get(owner, mt->entries[i].slot - 1, rwx, addr);

	return ERR_SUCCESS;
}

/**
 * Disables a memory capability by clearing the PMP slot.
 */
int mem_pmp_clear(mem_table_t *mt, pid_t owner, index_t i)
{
	if (UNLIKELY(!mem_valid_access(mt, owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Clear the PMP slot if it is set.
	if (mt->entries[i].slot != 0) {
		proc_pmp_clear(owner, mt->entries[i].slot - 1);
		mt->entries[i].slot = 0;
	}

	return ERR_SUCCESS;
}

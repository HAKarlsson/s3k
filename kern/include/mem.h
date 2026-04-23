// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Henrik Karlsson <hakarlsson@proton.me>
//
// mem.h -- Memory capability management for the s3k microkernel.
//
// Each memory capability represents an owned, permission-constrained view of a
// contiguous physical address range. Capabilities are arranged in a flat table
// (mem_table) partitioned into fixed-size subtables of MAX_MEMORY_FUEL slots.
// Within each subtable, slot 0 is the root capability; derived children occupy
// the slots that follow it. The fields cfree and csize together encode how many
// slots remain free for further derivation and how large the subtable is.
//
// Invariants maintained by this module:
//
//   Capability states:
//     - live:    owner != INVALID_PID. The capability is owned and usable.
//     - deleted: owner == INVALID_PID, cfree > 0. Owner cleared by
//                mem_delete; cfree/csize preserved for revoke traversal.
//     - free:    fully zeroed (cfree == 0). Belongs to an ancestor's free pool.
//
//   Subtable layout:
//     - The table is divided into NUM_MEMORY_CAPS subtables of MAX_MEMORY_FUEL
//       slots each. Subtable i starts at index i * MAX_MEMORY_FUEL.
//     - Within a subtable rooted at index i:
//         * Slots [i+1, i+cfree)  are free slots.
//         * Slots [i+cfree, i+csize) are live or deleted descendants of i
//           (all generations), in the order they were derived.
//         * cfree <= csize <= MAX_MEMORY_FUEL at all times.
//     - Every live or deleted cap has cfree > 0 (fuel > 0 enforced at
//       derivation; cfree only grows during revocation).
//     - { [i, i+cfree) } over all live/deleted caps partitions the subtable,
//       encoding the tree as a flat array (revoke needs no explicit links).
//
//   Address range:
//     - For every live cap: base < base + size (no wrap; enforced at init and derivation).
//     - child.base >= parent.base  and  child.base + child.size <= parent.base + parent.size.
//
//   Permissions:
//     - rwx is either MEM_PERM_NONE or a combination that includes MEM_PERM_R
//       (write-only and execute-only are not permitted).
//     - A child's permissions are always a subset of its parent's:
//       (child.rwx & parent.rwx) == child.rwx.
//
//   PMP:
//     - A live capability's slot field is 0 if no PMP entry is active, or a
//       1-based PMP slot index otherwise.
//     - The PMP entry's region fits within the capability's address range and
//       its permissions are a subset of the capability's permissions.
//     - PMP mappings are cleared before a capability changes owner (transfer)
//       or transitions to deleted or free (delete/revoke).

#pragma once

#include "types.h"

typedef struct {
	pid_t owner;	 ///< Owning process (INVALID_PID if deleted).
	fuel_t cfree;	 ///< Free slots in this capability's subtable.
	fuel_t csize;	 ///< Total slots allocated to this capability's subtable.
	pmp_slot_t slot; ///< Active PMP slot (1-based; 0 = no mapping).
	mem_perm_t rwx;	 ///< Permission bits (MEM_PERM_{R,W,X}).
	mem_addr_t base; ///< Start address of the memory region.
	mem_addr_t size; ///< Size of the memory region in bytes.
} __attribute__((aligned(sizeof(word_t)))) mem_t;

/**
 * Initialize the memory capability table.
 *
 * Each entry in @p init_mems becomes the root of one subtable at
 * index i * MAX_MEMORY_FUEL with cfree == csize == MAX_MEMORY_FUEL.
 */
void mem_init(mem_t init_mems[]);

/** Return true if @p i is in bounds and owned by @p owner. */
bool mem_valid_access(pid_t owner, index_t i);

/**
 * Transfer capability @p i from @p owner to @p new_owner.
 *
 * Clears any active PMP mapping before changing owner.
 * Returns ERR_INVALID_ACCESS if the access check fails.
 */
int mem_transfer(pid_t owner, index_t i, pid_t new_owner);

/**
 * Read the capability at @p i + @p offset in the same subtable.
 *
 * Returns ERR_INVALID_ACCESS, or ERR_INVALID_ARGUMENT if @p offset >= csize.
 */
int mem_introspect(pid_t owner, index_t i, fuel_t offset, mem_t *cap);

/**
 * Derive a child capability from @p i.
 *
 * Consumes @p fuel slots from @p i's free pool. Child region and permissions
 * must be subsets of the parent's. Returns the child's table index on success,
 * ERR_INVALID_ACCESS, or ERR_INVALID_ARGUMENT.
 */
int mem_derive(pid_t owner, index_t i, pid_t new_owner, fuel_t fuel, mem_perm_t rwx, mem_addr_t base, mem_addr_t size);

/**
 * Revoke all descendants of @p i, reclaiming their slots.
 *
 * Preemptible; returns remaining descendant count (0 = fully revoked).
 * Returns ERR_INVALID_ACCESS on failure.
 */
int mem_revoke(pid_t owner, index_t i);

/**
 * Delete capability @p i (set owner to INVALID_PID).
 *
 * Preserves subtable layout fields for parent revoke traversal.
 * Clears any active PMP mapping. Returns ERR_INVALID_ACCESS on failure.
 */
int mem_delete(pid_t owner, index_t i);

/**
 * Install a NAPOT PMP entry for capability @p i.
 *
 * @p slot is 1-based; region and permissions must fit within the capability.
 * Replaces any existing mapping; reconfigures in place if @p slot matches the
 * current slot. Returns ERR_INVALID_ACCESS, ERR_INVALID_ARGUMENT, or
 * ERR_SLOT_IN_USE if @p slot is held by another capability.
 */
int mem_pmp_set(pid_t owner, index_t i, pmp_slot_t slot, mem_perm_t rwx, pmp_addr_t addr);

/**
 * Read the active PMP configuration for capability @p i.
 *
 * If no mapping is active, *@p slot, *@p rwx, and *@p addr are all set to 0.
 * Returns ERR_INVALID_ACCESS on failure.
 */
int mem_pmp_get(pid_t owner, index_t i, pmp_slot_t *slot, mem_perm_t *rwx, pmp_addr_t *addr);

/** Clear the active PMP mapping for capability @p i, if any. */
int mem_pmp_clear(pid_t owner, index_t i);

// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Henrik Karlsson <hakarlsson@proton.me>

#include "mem.h"

#include "macro.h"
#include "pmp.h"
#include "preempt.h"
#include "proc.h"

// Flat table of all memory capabilities, partitioned into NUM_MEMORY_CAPS
// subtables of MAX_MEMORY_FUEL slots each.
static mem_t mem_table[MEM_TABLE_SIZE];

_Static_assert(NUM_MEMORY_CAPS * MAX_MEMORY_FUEL == MEM_TABLE_SIZE,
	       "mem_table is too small for NUM_MEMORY_CAPS * MAX_MEMORY_FUEL entries");

// MEM_PERM_NONE or any combination including MEM_PERM_R (no write-only/execute-only).
static bool perm_valid(word_t rwx)
{
	return ((rwx & MEM_PERM_R) == MEM_PERM_R) || (rwx == MEM_PERM_NONE);
}

void mem_init(mem_t init_mems[])
{
	for (index_t i = 0; i < NUM_MEMORY_CAPS; ++i) {
		KASSERT(perm_valid(init_mems[i].rwx));
		KASSERT(init_mems[i].base < init_mems[i].base + init_mems[i].size);
		for (index_t k = 0; k < i; ++k) { // no overlap with entries [0, i)
			KASSERT(init_mems[i].base >= init_mems[k].base + init_mems[k].size
				|| init_mems[k].base >= init_mems[i].base + init_mems[i].size);
		}
		mem_table[i * MAX_MEMORY_FUEL] = (mem_t){
			.owner = INIT_PID,
			.base = init_mems[i].base,
			.size = init_mems[i].size,
			.rwx = init_mems[i].rwx,
			.cfree = MAX_MEMORY_FUEL,
			.csize = MAX_MEMORY_FUEL,
		};
	}
}

bool mem_valid_access(pid_t owner, index_t i)
{
	KASSERT(owner != INVALID_PID);
	return (i < ARRAY_SIZE(mem_table)) && (mem_table[i].owner == owner);
}

// Returns true if a child can be derived from @p parent: child region and permissions
// are subsets of the parent's, and parent has at least @p fuel free slots (fuel > 0).
static bool cap_derivable(const mem_t *parent, fuel_t fuel, word_t rwx, word_t base, word_t size)
{
	KASSERT(parent->owner != INVALID_PID);

	return (base < base + size) && (parent->cfree > fuel) && (parent->base <= base)
	       && (base + size <= parent->base + parent->size) && ((parent->rwx & rwx) == rwx) && (fuel > 0)
	       && perm_valid(rwx);
}

// Returns true if NAPOT @p addr and @p rwx are valid for @p cap:
// decoded region and permissions are subsets of cap's, slot in range, no NAPOT overflow.
static bool pmp_args_valid(const mem_t *cap, word_t slot, mem_perm_t rwx, pmp_addr_t addr)
{
	KASSERT(cap->owner != INVALID_PID);

	if (addr > PMP_ADDR_MAX)
		return false;

	word_t base = pmp_napot_decode_base(addr);
	word_t size = pmp_napot_decode_size(addr);

	return (slot > 0) && (slot <= MAX_PMP_SLOT) && (cap->base <= base) && (base + size <= cap->base + cap->size)
	       && ((rwx & cap->rwx) == rwx) && perm_valid(rwx);
}

int mem_transfer(pid_t owner, index_t i, pid_t new_owner)
{
	KASSERT(new_owner != INVALID_PID);
	if (UNLIKELY(!mem_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	if (mem_table[i].slot != 0) {
		proc_pmp_clear(owner, mem_table[i].slot - 1);
		mem_table[i].slot = 0;
	}

	mem_table[i].owner = new_owner;

	return ERR_SUCCESS;
}

int mem_introspect(pid_t owner, index_t i, fuel_t offset, mem_t *cap)
{
	if (UNLIKELY(!mem_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	if (offset >= mem_table[i].csize) {
		return ERR_INVALID_ARGUMENT;
	}

	KASSERT(i + offset < MEM_TABLE_SIZE);

	*cap = mem_table[i + offset];
	return ERR_SUCCESS;
}

int mem_derive(pid_t owner, index_t i, pid_t new_owner, fuel_t fuel, mem_perm_t rwx, mem_addr_t base, mem_addr_t size)
{
	KASSERT(new_owner != INVALID_PID);
	if (UNLIKELY(!mem_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	if (UNLIKELY(!cap_derivable(&mem_table[i], fuel, rwx, base, size))) {
		return ERR_INVALID_ARGUMENT;
	}

	mem_table[i].cfree -= fuel;

	index_t child_idx = i + mem_table[i].cfree;
	KASSERT(child_idx < MEM_TABLE_SIZE);

	mem_table[child_idx] = (mem_t){
		.owner = new_owner,
		.cfree = fuel,
		.csize = fuel,
		.slot = 0,
		.rwx = rwx,
		.base = base,
		.size = size,
	};

	return child_idx;
}

int mem_revoke(pid_t owner, index_t i)
{
	if (UNLIKELY(!mem_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	while (mem_table[i].cfree < mem_table[i].csize) {
		index_t child_idx = i + mem_table[i].cfree;
		KASSERT(mem_table[i].cfree > 0);
		KASSERT(child_idx < MEM_TABLE_SIZE);

		mem_t child = mem_table[child_idx];
		mem_table[i].cfree += child.cfree;

		if (child.slot != 0) {
			proc_pmp_clear(child.owner, child.slot - 1);
		}
		mem_table[child_idx] = (mem_t){0};

		if (UNLIKELY(preempt()))
			break;
	}

	return mem_table[i].csize - mem_table[i].cfree; // 0 = done
}

int mem_delete(pid_t owner, index_t i)
{
	if (UNLIKELY(!mem_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	if (mem_table[i].slot != 0) {
		proc_pmp_clear(owner, mem_table[i].slot - 1);
		mem_table[i].slot = 0;
	}

	mem_table[i].owner = INVALID_PID;

	return ERR_SUCCESS;
}

int mem_pmp_set(pid_t owner, index_t i, pmp_slot_t slot, mem_perm_t rwx, pmp_addr_t addr)
{
	if (UNLIKELY(!mem_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	if (UNLIKELY(!pmp_args_valid(&mem_table[i], slot, rwx, addr))) {
		return ERR_INVALID_ARGUMENT;
	}

	// Reject if the target slot is occupied by a different capability.
	if (UNLIKELY(mem_table[i].slot != slot && proc_pmp_is_set(owner, slot - 1))) {
		return ERR_SLOT_IN_USE;
	}

	if (mem_table[i].slot != 0 && mem_table[i].slot != slot) {
		proc_pmp_clear(owner, mem_table[i].slot - 1);
	}

	proc_pmp_set(owner, slot - 1, rwx, addr);
	mem_table[i].slot = slot;

	return ERR_SUCCESS;
}

int mem_pmp_get(pid_t owner, index_t i, pmp_slot_t *slot, mem_perm_t *rwx, pmp_addr_t *addr)
{
	if (UNLIKELY(!mem_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	if (mem_table[i].slot == 0) {
		*slot = 0;
		*rwx = 0;
		*addr = 0;
		return ERR_SUCCESS;
	}

	*slot = mem_table[i].slot;
	proc_pmp_get(owner, mem_table[i].slot - 1, rwx, addr);

	return ERR_SUCCESS;
}

int mem_pmp_clear(pid_t owner, index_t i)
{
	if (UNLIKELY(!mem_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	if (mem_table[i].slot != 0) {
		proc_pmp_clear(owner, mem_table[i].slot - 1);
		mem_table[i].slot = 0;
	}

	return ERR_SUCCESS;
}

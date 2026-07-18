# S3K Architectural Description

## Error Codes

Every S3K system call returns an integer error code:

| Code | Description |
|------|-------------|
| `> 0` | Success (capability index or value) |
| `0` | Success (no return value) |
| `-1` | Invalid access |
| `-2` | Invalid argument |
| `-3` | Invalid system call |
| `-4` | Invalid capability |

## Capability System

S3K uses capabilities - unforgeable tokens granting specific rights to resources (memory, time slices, IPC channels) - to enforce strict access control and isolation between processes.

The capability system revolves around a global capability table per type, indexed by position. Processes can only access capabilities they own. Each capability controls a contiguous table subrange:

- `owner`: Process ID of the capability owner. 0 indicates unowned.
- `csize`: Total number of slots controlled by the capability.
- `cfree`: Number of free slots available for deriving children.

For a capability at index `i`:
- Controlled range (crange): `[i, i + csize - 1]`
- Free range (frange): `[i, i + cfree - 1]`

The free ranges of all capabilities partition the table: each slot belongs to exactly one capability's free range.

Capabilities can be derived, revoked, deleted, and granted. Derivation creates a new capability in the latter part of the parent's free range. Revocation reclaims derived children, expanding the parent's free range. Deletion disowns a capability without affecting children. Granting transfers ownership to another process.

The capability table can be seen as a nested stack of capability tables, where each capability controls a subrange of the table.
A derivation operation effectively "pushes" a new capability table onto the free range, while revocation "pops" it off, reclaiming its slots.

### Capability Derivation

When a process derives a new capability of size `s` from a parent `c` at index `i`, these conditions must hold:

- The parent is owned by the calling process.
- `0 < s <= c.cfree`.
- The new capability is placed in the latter `s` free slots of `c`'s free range.

The pseudo-code for deriving a new capability is as follows:
```c
err_t cap_derive(pid_t owner, cslot_t i, cslot_t csize, ...) {
	// Check that the parent capability is owned by the calling process
	if (i >= CTABLE_SIZE || ctable[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	// 0 < csize <= ctable[i].cfree
	if (csize == 0 || csize >= ctable[i].cfree) {
		return ERR_INVALID_ARGUMENT; // Invalid size
	}
	cslot_t j = i + ctable[i].cfree - csize; // New capability index
	// Initialize the new capability at index j
	ctable[j] = (cap_t){.owner = owner, .csize = csize, .cfree = csize, ...};
	// Pushing the new free range of j onto the parent's free range.
	ctable[i].cfree -= csize;
	return j;
}
```

### Capability Revocation

Capability revocation reclaims derived children by moving their slots back to the parent's free range. Revocation requires `cfree < csize` (children exist). When revoking a capability at index `i`:

- All children of the child at `i + cfree` are inherited by the parent.
- The parent's `cfree` expands to include the reclaimed slots.
- The child capability is cleared.

The pseudo-code for revoking a capability is as follows:
```c
err_t cap_revoke(pid_t owner, cslot_t i) {
	// Check that the capability is owned by the calling process
	if (i >= CTABLE_SIZE || ctable[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	
	// Check that capabilities have children to revoke.
	if (ctable[i].cfree < ctable[i].csize) {
		// First child index.
		cslot_t j = i + ctable[i].cfree; 

		// Inherit the children of the child capability.
		// Popping the child's free range back to the parent's free range.
		ctable[i].cfree += ctable[j].cfree; 

		// Clear the child capability.
		ctable[j] = (cap_t){0}; 
	}

	// Return the number of children remaining.
	return ctable[i].csize - ctable[i].cfree;
}
```

### Capability Deletion

Capability deletion disowns a capability, rendering it inaccessible to its owner. Only the owner can delete. Deletion does not affect children; the parent's `cfree` remains unchanged.

The pseudo-code for deleting a capability is as follows:
```c
err_t cap_delete(pid_t owner, cslot_t i) {
	// Check that the capability is owned by the calling process
	if (i >= CTABLE_SIZE || ctable[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	ctable[i].owner = 0; // Disown the capability.
	return 0; // Success
}
```

### Capability Granting

Capability granting transfers ownership from one process to another. Only the owner can grant a capability.

The pseudo-code for granting a capability is as follows:
```c
err_t cap_grant(pid_t owner, cslot_t i, pid_t recipient) {
	// Check that the capability is owned by the calling process
	if (i >= CTABLE_SIZE || ctable[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	// Grant the capability to the recipient process
	ctable[i].owner = recipient;
	return 0; // Success
}
```

## Time-Slice Capabilities

Time-slice capabilities grant processes the right to execute on a specific hart for a designated time window within the major frame. They parallel the general capability system but operate on time rather than table slots:

- `hart`: The hardware thread ID.
- `base`: Start slot in the major frame.
- `size`: Total time-slice length (analogous to `csize`).
- `free`: Unallocated length (analogous to `cfree`).
- `enabled`: Whether the time slice is active.

For a time-slice capability:
- Controlled frame: `[base, base + size - 1]`
- Free frame: `[base, base + free - 1]`

### Time-Slice Derivation

When a process derives a new time-slice capability of length `l` from a parent at index `i`, these conditions must hold:

- The parent is owned by the calling process.
- `0 < l <= parent.free`.
- The new capability is placed at the end of the parent's free frame.

The pseudo-code for deriving a new time slice capability is as follows:
```c
err_t tsl_derive(pid_t owner, cslot_t i, cslot_t csize, tslot_t tsize, bool tenabled) {
	if (i >= TSL_TABLE_SIZE || tslice_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	if (csize == 0 || csize >= tslice_table[i].cfree) {
		return ERR_INVALID_ARGUMENT; // Invalid capability size
	}
	if (tsize == 0 || tsize > tslice_table[i].size) {
		return ERR_INVALID_ARGUMENT; // Invalid time slice size
	}

	cslot_t j = i + tslice_table[i].cfree - csize; // New capability index
	// Initialize the new time slice capability at index j
	tslice_table[j] = (tslice_cap_t){
		.owner = owner,
		.csize = csize,
		.cfree = csize,
		.hart = tslice_table[i].hart,
		.base = tslice_table[i].base + tslice_table[i].free - tsize,
		.size = tsize,
		.free = tsize,
		.enabled = tenabled
	};
	// Update the parent time slice capability's free length
	tslice_table[i].cfree -= csize;
	tslice_table[i].free -= tsize;

	// Update the scheduling table to reflect the new time slice capability
	sched_update(&tslice_table[i]); // Update the scheduling entry for the parent time slice capability.
	sched_update(&tslice_table[j]); // Update the scheduling entry for the new time slice capability.

	// Return the index of the new time slice capability
	return j;
}
```

### Time-Slice Revocation

The pseudo-code for revoking a time slice capability is as follows:
```c
err_t tsl_revoke(pid_t owner, cslot_t i) {
	if (i >= TSL_TABLE_SIZE || tslice_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	// Check that the time slice capability has children to revoke.
	if (tslice_table[i].cfree < tslice_table[i].csize) {
		// First child index.
		cslot_t j = i + tslice_table[i].cfree;
		// Inherit the children of the child time slice capability.
		// Reclaim the free length of the child time slice capability.
		tslice_table[i].cfree += tslice_table[j].cfree;
		tslice_table[i].free += tslice_table[j].free;

		sched_clear(&tslice_table[j]); // Clear the scheduling entry for the child time slice capability.
		sched_update(&tslice_table[i]); // Update the scheduling entry for the parent time slice capability.

		// Clear the child time slice capability.
		tslice_table[j] = (tslice_cap_t){0};
	}
	// Return the number of children remaining.
	return tslice_table[i].csize - tslice_table[i].cfree;
}
```

### Time slice Deletion

The pseudo-code for deleting a time slice capability is as follows:
```c
err_t tsl_delete(pid_t owner, cslot_t i) {
	if (i >= TSL_TABLE_SIZE || tslice_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	// Disown the time slice capability.
	tslice_table[i].owner = 0;

	sched_update(&tslice_table[i]); // Update the scheduling entry to reflect the deletion.

	return 0; // Success
}
```

### Time slice Granting

The pseudo-code for granting a time slice capability is as follows:
```c
err_t tsl_grant(pid_t owner, cslot_t i, pid_t recipient) {
	if (i >= TSL_TABLE_SIZE || tslice_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	// Grant the time slice capability to the recipient process
	tslice_table[i].owner = recipient;
	sched_update(&tslice_table[i]); // Update the scheduling entry to reflect the grant.
	return 0; // Success
}
```

## Memory Capabilities

S3K memory capabilities grant processes access to specific physical memory regions, with permissions enforced by the Physical Memory Protection (PMP) unit. Each memory capability includes:

- `base`: Base physical address of the memory region.
- `size`: Size of the memory region.
- `perm`: Permissions (read, write, execute) for the memory region.
- `slot`: PMP slot assigned to the memory region. 0 indicates no PMP slot assigned.

### Memory Capability Derivation

When a process derives a new memory capability from a parent, the following conditions must hold:

- The parent capability's owner is the calling process.
- The new capability's base address and size must be within the parent's controlled range.
- The new capability's permissions must be a subset of the parent's permissions.

The pseudo-code for deriving a new memory capability is as follows:
```c
err_t mem_derive(pid_t owner, cslot_t i, cslot_t csize, paddr_t base, size_t size, mem_perm_t perm) {
	if (i >= MEM_TABLE_SIZE || mem_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}

	// Check integer overflow for base + size
	if (base + size < base) {
		return ERR_INVALID_ARGUMENT; // Overflow
	}

	// Check that the new memory region is within the parent's controlled range.
	if (base < mem_table[i].base || (base + size) > (mem_table[i].base + mem_table[i].size)) {
		return ERR_INVALID_ARGUMENT; // Out of bounds
	}

	// Check that the new permissions are a subset of the parent's permissions.
	if ((perm & ~mem_table[i].perm) != 0) {
		return ERR_INVALID_ARGUMENT; // Invalid permissions
	}

	// New capability index is at the end of the parent's free range.
	cslot_t j = i + mem_table[i].cfree - csize;

	// Initialize the new memory capability at index j
	mem_table[j] = (mem_cap_t){
		.owner = owner,
		.base = base,
		.size = size,
		.perm = perm,
		.slot = 0,
	};

	mem_table[i].cfree -= csize; // Update parent's free count

	return j; // Return the slot index of the new capability.
}
```

### Memory Capability Revocation

The pseudo-code for revoking a memory capability is as follows:
```c
err_t mem_revoke(pid_t owner, cslot_t i) {
	if (i >= MEM_TABLE_SIZE || mem_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	// Check that the memory capability has children to revoke.
	if (mem_table[i].cfree < mem_table[i].csize) {
		// First child index.
		cslot_t j = i + mem_table[i].cfree;

		// Inherit the children of the child memory capability.
		mem_table[i].cfree += mem_table[j].cfree;

		pmp_clear(mem_table[j].owner, mem_table[j].slot); // Clear the PMP slot if assigned.

		// Clear the child memory capability.
		mem_table[j] = (mem_cap_t){0};
	}
	// Return the number of children remaining.
	return mem_table[i].csize - mem_table[i].cfree;
}
```

### Memory Capability Deletion

The pseudo-code for deleting a memory capability is as follows:
```c
err_t mem_delete(pid_t owner, cslot_t i) {
	if (i >= MEM_TABLE_SIZE || mem_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	pmp_clear(mem_table[i].owner, mem_table[i].slot); // Clear the PMP slot if assigned.
	mem_table[i].owner = 0; // Disown the memory capability.
	return 0; // Success
}
```

### Memory Capability Granting

The pseudo-code for granting a memory capability is as follows:
```c
err_t mem_grant(pid_t owner, cslot_t i, pid_t recipient) {
	if (i >= MEM_TABLE_SIZE || mem_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	pmp_clear(owner, mem_table[i].slot); // Clear the PMP slot for the previous owner.
	mem_table[i].slot = 0; // Reset the PMP slot for the new owner.
	mem_table[i].owner = recipient; // Grant the memory capability to the recipient process.
	return 0; // Success
}
```

### Memory Capability PMP Configuration

The pseudo-code for getting and setting PMP configuration for a memory capability is as follows:
```c
err_t mem_pmp_get(pid_t owner, cslot_t i, pmp_slot_t *slot, mem_perm_t *perm, pmp_addr_t *addr) {
	if (i >= MEM_TABLE_SIZE || mem_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	*slot = mem_table[i].slot;
	pmp_get(mem_table[i].owner, mem_table[i].slot, perm, addr); // Retrieve the PMP configuration for the memory capability.
	return 0; // Success
}
```

The pseudo-code for setting PMP configuration for a memory capability is as follows:
```c
err_t mem_pmp_set(pid_t owner, cslot_t i, pmp_slot_t slot, mem_perm_t perm, pmp_addr_t addr) {
	if (i >= MEM_TABLE_SIZE || mem_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}

	/// Address must be within the 32-bit physical address space.
	if (addr & ~0xFFFFFFFFull != 0) {
		return ERR_INVALID_ARGUMENT; // Address out of range
	}

	paddr_t pbase = pmp_napot_base(addr);
	paddr_t psize = pmp_napot_size(addr);

	// Check that the new PMP configuration is within the memory capability's bounds.
	if (pbase < mem_table[i].mbase || (pbase + psize) > (mem_table[i].mbase + mem_table[i].msize)) {
		return ERR_INVALID_ARGUMENT; // Out of bounds
	}

	// Check that the new permissions are a subset of the memory capability's permissions.
	if ((perm & ~mem_table[i].mperm) != 0) {
		return ERR_INVALID_ARGUMENT; // Invalid permissions
	}

	// Check valid PMP slot number.
	if (slot == 0 || slot > PMP_SLOT_COUNT) {
		return ERR_INVALID_ARGUMENT; // Invalid PMP slot
	}

	pmp_clear(mem_table[i].owner, mem_table[i].slot); // Clear the previous PMP entry if assigned.

	// Update the memory capability's PMP slot and permissions.
	mem_table[i].slot = slot;
	pmp_set(mem_table[i].owner, slot, perm, addr); // Configure the PMP entry.

	return 0; // Success
}
```

The pseudo-code for clearing PMP configuration for a memory capability is as follows:
```c
err_t mem_pmp_clear(pid_t owner, cslot_t i) {
	if (i >= MEM_TABLE_SIZE || mem_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	pmp_clear(mem_table[i].owner, mem_table[i].slot); // Clear the PMP entry for the memory capability.
	mem_table[i].slot = 0;
	return 0; // Success
}
```

## Monitor Capabilities

A monitor capability allows a process to control the execution of another process, including suspending, resuming, yielding, and manipulating register values. Each monitor capability includes:

- `pid`: Process ID of the monitor capability owner.

### Monitor Capability Derivation

The pseudo-code for deriving a new monitor capability is as follows:
```c
err_t mon_derive(pid_t owner, cslot_t i, cslot_t csize) {
	if (i >= MON_TABLE_SIZE || mon_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	if (csize == 0 || csize >= mon_table[i].cfree) {
		return ERR_INVALID_ARGUMENT; // Invalid size
	}
	cslot_t j = i + mon_table[i].cfree - csize; // New capability index
	// Initialize the new monitor capability at index j
	mon_table[j] = (mon_cap_t){.owner = owner, .csize = csize, .cfree = csize, .pid = mon_table[i].pid};
	mon_table[i].cfree -= csize; // Update parent's free count
	return j; // Return the slot index of the new monitor capability.
}
```

### Monitor Capability Revocation

The pseudo-code for revoking a monitor capability is as follows:
```c
err_t mon_revoke(pid_t owner, cslot_t i) {
	if (i >= MON_TABLE_SIZE || mon_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	// Check that the monitor capability has children to revoke.
	if (mon_table[i].cfree < mon_table[i].csize) {
		// First child index.
		cslot_t j = i + mon_table[i].cfree;

		// Inherit the children of the child monitor capability.
		mon_table[i].cfree += mon_table[j].cfree;

		// Clear the child monitor capability.
		mon_table[j] = (mon_cap_t){0};
	}
	// Return the number of children remaining.
	return mon_table[i].csize - mon_table[i].cfree;
}
```

### Monitor Capability Deletion

The pseudo-code for deleting a monitor capability is as follows:
```c
err_t mon_delete(pid_t owner, cslot_t i) {
	if (i >= MON_TABLE_SIZE || mon_table[i].owner != owner) {
		return ERR_INVALID_ACCESS; // Not the owner
	}
	mon_table[i].owner = 0; // Disown the monitor capability.
	return 0; // Success
}
```

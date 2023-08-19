#pragma once

#include "cap/types.h"
#include "proc/proc.h"

#include <stdint.h>

typedef enum {
	// Basic Info & Registers
	SYSCALL_INFO,	  // Retrieve basic system information
	SYSCALL_REG_GET,  // Get the value of a specific register
	SYSCALL_REG_SET,  // Set the value of a specific register
	SYSCALL_SYNC,	  // Synchronize with capabilities/scheduling
	SYSCALL_MEM_SYNC, // Synchronize with capabilities/scheduling

	// Capability Management
	SYSCALL_CAP_READ,   // Read the properties of a capability
	SYSCALL_CAP_MOVE,   // Move a capability to a different slot
	SYSCALL_CAP_DELETE, // Remove a capability from the system
	SYSCALL_CAP_REVOKE, // Revoke a derived capabilities
	SYSCALL_CAP_DERIVE, // Derive a new capability from an existing one

	// PMP Management
	SYSCALL_PMP_LOAD,   // Load a PMP capability into hardware
	SYSCALL_PMP_UNLOAD, // Unload a PMP capability from hardware

	// Monitor Management
	SYSCALL_MON_SUSPEND,	  // Suspend monitored process
	SYSCALL_MON_RESUME,	  // Resume monitored process
	SYSCALL_MON_REG_GET,	  // Get register of monitored process
	SYSCALL_MON_REG_SET,	  // Set register of monitored process
	SYSCALL_MON_CAP_READ,	  // Read a capability of monitored process
	SYSCALL_MON_CAP_TRANSFER, // Transfer a capability to/from monitored
				  // process
	SYSCALL_MON_PMP_LOAD,	  // Load a PMP capability in monitored process
	SYSCALL_MON_PMP_UNLOAD, // Unload a PMP capability in monitored process

	// Socket Operations
	SYSCALL_SOCK_SEND,  // Send message on IPC Socket (client)
	SYSCALL_SOCK_CALL,  // Send then wait to receive on IPC Socket (client)
	SYSCALL_SOCK_RECV,  // Wait to receive message on IPC Socket (server)
	SYSCALL_SOCK_REPLY, // Reply on IPC Socket (server)
	SYSCALL_SOCK_REPLY_RECV, // Reply then wait to receive message on IPC
				 // Socket (server)
} syscall_t;

void handle_syscall(proc_t *proc);

void syscall_info(proc_t *proc, uint64_t info);
void syscall_reg_get(proc_t *proc, uint64_t reg);
void syscall_reg_set(proc_t *proc, uint64_t reg, uint64_t val);
void syscall_sync(proc_t *proc);
void syscall_cap_read(proc_t *proc, uint64_t src);
void syscall_cap_move(proc_t *proc, uint64_t src, uint64_t dst);
void syscall_cap_delete(proc_t *proc, uint64_t src);
void syscall_cap_revoke(proc_t *proc, uint64_t src);
void syscall_cap_derive(proc_t *proc, uint64_t src, uint64_t dst, cap_t cap);

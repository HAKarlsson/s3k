/**
 * @file stack.h
 * @brief Definition of system calls and error codes.
 * @copyright MIT License
 * @author Henrik Karlsson (henrik10@kth.se)
 */
#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "proc.h"

#include <stdint.h>

/// @defgroup syscall-handle System Call Handling
/// @{

/// System call exceptions
enum excpt {
	EXCPT_NONE,	    ///< No exception.
	EXCPT_EMPTY,	    ///< Capability slot is empty.
	EXCPT_COLLISION,    ///< Capability slot is occupied.
	EXCPT_DERIVATION,   ///< Capability can not be derived.
	EXCPT_PREEMPTED,    ///< System call was preempted.
	EXCPT_SUSPENDED,    ///< Process was suspended
	EXCPT_MPID,	    ///< Bad PID for monitor operation.
	EXCPT_MBUSY,	    ///< Process busy.
	EXCPT_INVALID_CAP,  ///< Capability used for the system call is invalid.
	EXCPT_NO_RECEIVER,  ///< No receiver for the send call.
	EXCPT_SEND_CAP,	    ///< Something stops sending of capability.
	EXCPT_UNIMPLEMENTED ///< System call not implemented for specified
			    ///< capability.
};

enum syscall_nr {
	// Capabilityless syscalls
	SYSCALL_PROC, ///< Process local system call.
	// Capability syscalls
	SYSCALL_GETCAP, ///< Get capability description
	SYSCALL_MOVCAP, ///< Move capability
	SYSCALL_DELCAP, ///< Delete capability
	SYSCALL_REVCAP, ///< Revoke capability
	SYSCALL_DRVCAP, ///< Derive capability
	// Monitor syscalls
	SYSCALL_MSUSPEND, ///< Monitor suspend process
	SYSCALL_MRESUME,  ///< Monitor resume process
	SYSCALL_MGETREG,  ///< Monitor get register value
	SYSCALL_MSETREG,  ///< Monitor set register value
	SYSCALL_MGETCAP,  ///< Monitor get capability description
	SYSCALL_MTAKECAP, ///< Monitor take capability
	SYSCALL_MGIVECAP, ///< Monitor give capability
	// IPC syscalls
	SYSCALL_RECV,	  ///< Receive message/capability
	SYSCALL_SEND,	  ///< Send message/capability
	SYSCALL_SENDRECV, ///< Send then receive message/capability
};

// Simple system calls
/// Process local system call.
struct proc *syscall_proc(struct proc *proc, uint64_t a0, uint64_t a1,
			  uint64_t a2);

// Capability system calls
/// Get capability description.
struct proc *syscall_getcap(struct proc *proc, uint64_t i);
/// Get move capability.
struct proc *syscall_movcap(struct proc *proc, uint64_t i, uint64_t j);
/// Delete capability.
struct proc *syscall_delcap(struct proc *proc, uint64_t i);
/// Revoke capability.
struct proc *syscall_revcap(struct proc *proc, uint64_t i);
/// Derive capability.
struct proc *syscall_drvcap(struct proc *proc, uint64_t i, uint64_t j,
			    union cap cap);

// Monitor capability system calls
/// Suspend a process
struct proc *syscall_msuspend(struct proc *proc, uint64_t mon, uint64_t pid);
/// Resume a process
struct proc *syscall_mresume(struct proc *proc, uint64_t mon, uint64_t pid);
/// Get a register of a process
struct proc *syscall_mgetreg(struct proc *proc, uint64_t mon, uint64_t pid,
			     uint64_t reg);
/// Set a register of a process
struct proc *syscall_msetreg(struct proc *proc, uint64_t mon, uint64_t pid,
			     uint64_t reg, uint64_t val);
/// Read a capability of a process
struct proc *syscall_mgetcap(struct proc *proc, uint64_t mon, uint64_t pid,
			     uint64_t i);
/// Take a capability from a process
struct proc *syscall_mtakecap(struct proc *proc, uint64_t mon, uint64_t pid,
			      uint64_t i, uint64_t j);
/// Give a capability to a process
struct proc *syscall_mgivecap(struct proc *proc, uint64_t mon, uint64_t pid,
			      uint64_t i, uint64_t j);

// IPC capability system calls
/// Receive a message.
struct proc *syscall_recv(struct proc *proc, uint64_t recv_idx,
			  uint64_t dest_cap);
/// Send a message.
struct proc *syscall_send(struct proc *proc, uint64_t send_idx, uint64_t msg0,
			  uint64_t msg1, uint64_t msg2, uint64_t msg3,
			  uint64_t src_cap, uint64_t yield);
/// Send then receive a message.
struct proc *syscall_sendrecv(struct proc *proc, uint64_t sendrecv_idx,
			      uint64_t msg0, uint64_t msg1, uint64_t msg2,
			      uint64_t msg3, uint64_t src_cap,
			      uint64_t dest_cap);
/// @}

#endif /* __SYSCALL_H__ */

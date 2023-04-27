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

void syscall_handler(uint64_t idx, uint64_t inv, uint64_t msg0, uint64_t msg1,
		     uint64_t msg2, uint64_t msg3);

#endif /* __SYSCALL_H__ */

#pragma once
/**
 * @file syscall.h
 * @brief Definition of system calls and error codes.
 * @copyright MIT License
 * @author Henrik Karlsson (henrik10@kth.se)
 */
#include "cap_types.h"
#include "proc.h"

#include <stdint.h>

typedef enum {
	SYSCALL_GET_INFO,
	SYSCALL_GET_REG,
	SYSCALL_SET_REG,
	SYSCALL_YIELD,
	SYSCALL_GET_CAP,
	SYSCALL_MOVE_CAP,
	SYSCALL_DELETE_CAP,
	SYSCALL_REVOKE_CAP,
	SYSCALL_DERIVE_CAP,
	SYSCALL_PMP_SET,
	SYSCALL_PMP_CLEAR,
	SYSCALL_MONITOR_SUSPEND,
	SYSCALL_MONITOR_RESUME,
	SYSCALL_MONITOR_GET_REG,
	SYSCALL_MONITOR_SET_REG,
	SYSCALL_MONITOR_GET_CAP,
	SYSCALL_MONITOR_TAKE_CAP,
	SYSCALL_MONITOR_GIVE_CAP,
	SYSCALL_MONITOR_PMP_SET,
	SYSCALL_MONITOR_PMP_CLEAR,
	SYSCALL_SOCKET_SEND,
	SYSCALL_SOCKET_RECV,
	SYSCALL_SOCKET_SENDRECV,
} syscall_nr_t;

void syscall_handler(void) __attribute__((noreturn));

excpt_t syscall_get_info(uint64_t info);
excpt_t syscall_get_reg(uint64_t regid);
excpt_t syscall_set_reg(uint64_t regid, uint64_t value);
excpt_t syscall_yield(uint64_t until);
excpt_t syscall_get_cap(uint64_t cidx);
excpt_t syscall_move_cap(uint64_t src_cidx, uint64_t dest_cidx);
excpt_t syscall_delete_cap(uint64_t cidx);
excpt_t syscall_revoke_cap(uint64_t cidx);
excpt_t syscall_derive_cap(uint64_t orig_cidx, uint64_t dest_cidx, cap_t new_cap);
excpt_t syscall_pmp_set(uint64_t pmp_cidx, uint64_t index);
excpt_t syscall_pmp_clear(uint64_t pmp_cidx);
excpt_t syscall_monitor_suspend(uint64_t mon_cidx, uint64_t pid);
excpt_t syscall_monitor_resume(uint64_t mon_cidx, uint64_t pid);

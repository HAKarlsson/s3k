/**
 * @file syscall.h
 * @brief Definition of system calls and error codes.
 * @copyright MIT License
 * @author Henrik Karlsson (henrik10@kth.se)
 */
#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "proc.h"

#include <stdint.h>

#define SYSCALL_GET_INFO 0
#define SYSCALL_GET_REG 1
#define SYSCALL_SET_REG 2
#define SYSCALL_YIELD 3
#define SYSCALL_READ_CAP 4
#define SYSCALL_MOVE_CAP 5
#define SYSCALL_DELETE_CAP 6
#define SYSCALL_REVOKE_CAP 7
#define SYSCALL_DERIVE_CAP 8
// Capbility invocations below
#define SYSCALL_PMP_SET 9
#define SYSCALL_PMP_CLEAR 10
#define SYSCALL_SOCKET_RECV 11
#define SYSCALL_SOCKET_SEND 12
#define SYSCALL_SOCKET_SENDRECV 13
#define SYSCALL_MONITOR_SUSPEND 14
#define SYSCALL_MONITOR_RESUME 15
#define SYSCALL_MONITOR_GET_REG 16
#define SYSCALL_MONITOR_SET_REG 17
#define SYSCALL_MONITOR_READ_CAP 18
#define SYSCALL_MONITOR_GIVE_CAP 19
#define SYSCALL_MONITOR_TAKE_CAP 20
#define SYSCALL_MONITOR_PMP_SET 21
#define SYSCALL_MONITOR_PMP_CLEAR 22

void syscall_handler(uint64_t sysnr, uint64_t a1, uint64_t a2, uint64_t a3,
		     uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7);

#endif /* __SYSCALL_H__ */

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

#define SYSCALL_GETINFO 0
#define SYSCALL_GETREG 1
#define SYSCALL_SETREG 2
#define SYSCALL_YIELD 3
#define SYSCALL_GETCAP 4
#define SYSCALL_MOVCAP 5
#define SYSCALL_DELCAP 6
#define SYSCALL_REVCAP 7
#define SYSCALL_DRVCAP 8
#define SYSCALL_INVCAP 9

void syscall_handler(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
		     uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7);

#endif /* __SYSCALL_H__ */

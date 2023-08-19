#pragma once

/* Register offsets */
#define REG_PC 0
#define REG_RA 1
#define REG_SP 2
#define REG_GP 3
#define REG_TP 4
#define REG_T0 5
#define REG_T1 6
#define REG_T2 7
#define REG_S0 8
#define REG_S1 9
#define REG_A0 10
#define REG_A1 11
#define REG_A2 12
#define REG_A3 13
#define REG_A4 14
#define REG_A5 15
#define REG_A6 16
#define REG_A7 17
#define REG_S2 18
#define REG_S3 19
#define REG_S4 20
#define REG_S5 21
#define REG_S6 22
#define REG_S7 23
#define REG_S8 24
#define REG_S9 25
#define REG_S10 26
#define REG_S11 27
#define REG_T3 28
#define REG_T4 29
#define REG_T5 30
#define REG_T6 31
#define REG_TPC 32
#define REG_TSP 33
#define REG_EPC 34
#define REG_ESP 35
#define REG_ECAUSE 36
#define REG_EVAL 37
#define N_REGS 38

/* Machine CSR constants */
#define MIP_MSIP 8
#define MIE_MSIE 8
#define MIP_MTIP 128
#define MIE_MTIE 128
#define MCAUSE_USER_ECALL 8
#define MSTATUS_MIE 8

/* System call number */
#define SYSCALL_GET_INFO 0
#define SYSCALL_GET_REG 1
#define SYSCALL_SET_REG 2
#define SYSCALL_YIELD 3
#define SYSCALL_GET_CAP 4
#define SYSCALL_MOVE_CAP 5
#define SYSCALL_DELETE_CAP 6
#define SYSCALL_REVOKE_CAP 7
#define SYSCALL_DERIVE_CAP 8
#define SYSCALL_PMP_SET 9
#define SYSCALL_PMP_CLEAR 10
#define SYSCALL_MONITOR_SUSPEND 11
#define SYSCALL_MONITOR_RESUME 12
#define SYSCALL_MONITOR_GET_REG 13
#define SYSCALL_MONITOR_SET_REG 14
#define SYSCALL_MONITOR_GET_CAP 15
#define SYSCALL_MONITOR_TAKE_CAP 16
#define SYSCALL_MONITOR_GIVE_CAP 17
#define SYSCALL_MONITOR_PMP_SET 18
#define SYSCALL_MONITOR_PMP_CLEAR 19
#define SYSCALL_SOCKET_SEND 20
#define SYSCALL_SOCKET_RECV 21
#define SYSCALL_SOCKET_SENDRECV 22
#define NUM_OF_SYSCALL 23

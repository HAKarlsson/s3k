#pragma once

#ifdef __ASSEMBLER__
#define _x(x) (x * 8)
#else /* __ASSEMBLER__ */
#define _x(x) (x)
#endif

/* Register offsets */
#define REG_PC _x(0)
#define REG_RA _x(1)
#define REG_SP _x(2)
#define REG_GP _x(3)
#define REG_TP _x(4)
#define REG_T0 _x(5)
#define REG_T1 _x(6)
#define REG_T2 _x(7)
#define REG_S0 _x(8)
#define REG_S1 _x(9)
#define REG_A0 _x(10)
#define REG_A1 _x(11)
#define REG_A2 _x(12)
#define REG_A3 _x(13)
#define REG_A4 _x(14)
#define REG_A5 _x(15)
#define REG_A6 _x(16)
#define REG_A7 _x(17)
#define REG_S2 _x(18)
#define REG_S3 _x(19)
#define REG_S4 _x(20)
#define REG_S5 _x(21)
#define REG_S6 _x(22)
#define REG_S7 _x(23)
#define REG_S8 _x(24)
#define REG_S9 _x(25)
#define REG_S10 _x(26)
#define REG_S11 _x(27)
#define REG_T3 _x(28)
#define REG_T4 _x(29)
#define REG_T5 _x(30)
#define REG_T6 _x(31)
#define REG_TPC _x(32)
#define REG_TSP _x(33)
#define REG_EPC _x(34)
#define REG_ESP _x(35)
#define REG_ECAUSE _x(36)
#define REG_EVAL _x(37)

#define NUM_OF_REGS 38
#define NUM_OF_GPR 32

/* PMP offset relative proc_t */
#define REG_PMPCFG0 _x(38)
#define REG_PMPADDR0 _x(39)
#define REG_PMPADDR1 _x(40)
#define REG_PMPADDR2 _x(41)
#define REG_PMPADDR3 _x(42)
#define REG_PMPADDR4 _x(43)
#define REG_PMPADDR5 _x(44)
#define REG_PMPADDR6 _x(45)
#define REG_PMPADDR7 _x(46)

/* Instrumentation offset relative proc_t */
#define REG_INSTRUMENT_WCET _x(47)

/* Capability types */
#define CAPTY_NULL 0
#define CAPTY_TIME 1
#define CAPTY_MEMORY 2
#define CAPTY_PMP 3
#define CAPTY_MONITOR 4
#define CAPTY_CHANNEL 5
#define CAPTY_SOCKET 6

/* Machine CSR constants */
#define MIP_MTIMER 0x8
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

/* Exception codes */
#define EXCPT_NONE 0
#define EXCPT_ERROR 1
#define EXCPT_INDEX 2
#define EXCPT_EMPTY 3
#define EXCPT_COLLISION 4
#define EXCPT_DERIVATION 5
#define EXCPT_INVALID_CAP 6
#define EXCPT_REG_INDEX 7
#define EXCPT_PMP_INDEX 8
#define EXCPT_PMP_COLLISION 9
#define EXCPT_MONITOR_PID 10
#define EXCPT_MONITOR_BUSY 11
#define EXCPT_MONITOR_INDEX 12
#define EXCPT_MONITOR_REG_INDEX 13
#define EXCPT_MONITOR_EMPTY 14
#define EXCPT_MONITOR_COLLISION 15
#define EXCPT_UNIMPLEMENTED 0xdeadbeef
#define EXCPT_PREEMPT -1

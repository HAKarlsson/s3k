#pragma once
#include <stdint.h>

typedef enum {
	S3K_EXCPT_NONE,
	S3K_EXCPT_ERROR,
	S3K_EXCPT_INDEX,
	S3K_EXCPT_EMPTY,
	S3K_EXCPT_COLLISION,
	S3K_EXCPT_DERIVATION,
	S3K_EXCPT_INVALID_CAP,
	S3K_EXCPT_REG_INDEX,
	S3K_EXCPT_PMP_INDEX,
	S3K_EXCPT_PMP_COLLISION,
	S3K_EXCPT_MONITOR_PID,
	S3K_EXCPT_MONITOR_BUSY,
	S3K_EXCPT_MONITOR_INDEX,
	S3K_EXCPT_MONITOR_REG_INDEX,
	S3K_EXCPT_MONITOR_EMPTY,
	S3K_EXCPT_MONITOR_COLLISION,
	S3K_EXCPT_UNIMPLEMENTED = 404,
	S3K_EXCPT_PREEMPT = -1
} s3k_excpt_t;

typedef enum {
	// Basic Info & Registers
	S3K_SYSCALL_INFO,    // Retrieve basic system information
	S3K_SYSCALL_REG_GET, // Get the value of a specific register
	S3K_SYSCALL_REG_SET, // Set the value of a specific register
	S3K_SYSCALL_SYNC,    // Synchronize with capabilities/scheduling

	// Capability Management
	S3K_SYSCALL_CAP_READ,	// Read the properties of a capability
	S3K_SYSCALL_CAP_MOVE,	// Move a capability to a different slot
	S3K_SYSCALL_CAP_DELETE, // Remove a capability from the system
	S3K_SYSCALL_CAP_REVOKE, // Revoke a derived capabilities
	S3K_SYSCALL_CAP_DERIVE, // Derive a new capability from an existing one

	// PMP Management
	S3K_SYSCALL_PMP_LOAD,	// Load a PMP capability into hardware
	S3K_SYSCALL_PMP_UNLOAD, // Unload a PMP capability from hardware

	// Monitor Management
	S3K_SYSCALL_MON_SUSPEND,      // Suspend monitored process
	S3K_SYSCALL_MON_RESUME,	      // Resume monitored process
	S3K_SYSCALL_MON_REG_GET,      // Get register of monitored process
	S3K_SYSCALL_MON_REG_SET,      // Set register of monitored process
	S3K_SYSCALL_MON_CAP_READ,     // Read a capability of monitored process
	S3K_SYSCALL_MON_CAP_TRANSFER, // Transfer a capability to/from monitored
				      // process
	S3K_SYSCALL_MON_PMP_LOAD, // Load a PMP capability in monitored process
	S3K_SYSCALL_MON_PMP_UNLOAD, // Unload a PMP capability in monitored
				    // process

	// Socket Operations
	S3K_SYSCALL_SOCK_SEND, // Send message on IPC Socket (client)
	S3K_SYSCALL_SOCK_CALL, // Send then wait to receive on IPC Socket
			       // (client)
	S3K_SYSCALL_SOCK_RECV, // Wait to receive message on IPC Socket (server)
	S3K_SYSCALL_SOCK_REPLY,	     // Reply on IPC Socket (server)
	S3K_SYSCALL_SOCK_REPLY_RECV, // Reply then wait to receive message on
				     // IPC Socket (server)
} s3k_syscall_t;

typedef enum {
	S3K_REG_PC,
	S3K_REG_RA,
	S3K_REG_SP,
	S3K_REG_GP,
	S3K_REG_TP,
	S3K_REG_T0,
	S3K_REG_T1,
	S3K_REG_T2,
	S3K_REG_S0,
	S3K_REG_S1,
	S3K_REG_A0,
	S3K_REG_A1,
	S3K_REG_A2,
	S3K_REG_A3,
	S3K_REG_A4,
	S3K_REG_A5,
	S3K_REG_A6,
	S3K_REG_A7,
	S3K_REG_S2,
	S3K_REG_S3,
	S3K_REG_S4,
	S3K_REG_S5,
	S3K_REG_S6,
	S3K_REG_S7,
	S3K_REG_S8,
	S3K_REG_S9,
	S3K_REG_S10,
	S3K_REG_S11,
	S3K_REG_T3,
	S3K_REG_T4,
	S3K_REG_T5,
	S3K_REG_T6,
	S3K_REG_TPC,
	S3K_REG_TSP,
	S3K_REG_EPC,
	S3K_REG_ESP,
	S3K_REG_ECAUSE,
	S3K_REG_EVAL
} s3k_reg_t;

// System calls
static inline uint64_t s3k_getpid(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_INFO;
	register uint64_t a0 __asm__("a0") = 0;
	__asm__ volatile("ecall" : "+r"(t0), "+r"(a0));
	return a0;
}

static inline uint64_t s3k_gettime(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_INFO;
	register uint64_t a0 __asm__("a0") = 1;
	__asm__ volatile("ecall" : "+r"(t0), "+r"(a0));
	return a0;
}

static inline uint64_t s3k_gettimeout(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_INFO;
	register uint64_t a0 __asm__("a0") = 2;
	__asm__ volatile("ecall" : "+r"(t0), "+r"(a0));
	return a0;
}

static inline uint64_t s3k_getwcet(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_INFO;
	register uint64_t a0 __asm__("a0") = 3;
	__asm__ volatile("ecall" : "+r"(t0), "+r"(a0));
	return a0;
}

static inline s3k_excpt_t s3k_getreg(uint64_t reg, uint64_t *val)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_REG;
	register uint64_t a0 __asm__("a0") = reg;
	__asm__ volatile("ecall" : "+r"(t0), "+r"(a0));
	*val = a0;
	return t0;
}

static inline s3k_excpt_t s3k_setreg(uint64_t reg, uint64_t val)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_SET_REG;
	register uint64_t a0 __asm__("a0") = reg;
	register uint64_t a1 __asm__("a1") = val;
	__asm__ volatile("ecall" : "+r"(t0) : "r"(a0), "r"(a1));
	return t0;
}

static inline void s3k_yield(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_YIELD;
	register uint64_t a0 __asm__("a0") = 0;
	__asm__ volatile("ecall" : "+r"(t0) : "r"(a0));
}

static inline s3k_excpt_t s3k_getcap(uint64_t i, s3k_cap_t *cap)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_CAP;
	register uint64_t a0 __asm__("a0") = i;
	__asm__ volatile("ecall" : "+r"(t0), "+r"(a0));
	if (t0 == S3K_EXCPT_NONE)
		cap->raw = a0;
	return t0;
}

static inline s3k_excpt_t s3k_movcap(uint64_t src, uint64_t dest)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MOVE_CAP;
	register uint64_t a0 __asm__("a0") = src;
	register uint64_t a1 __asm__("a1") = dest;
	__asm__ volatile("ecall" : "+r"(t0) : "r"(a0), "r"(a1));
	return t0;
}

static inline s3k_excpt_t s3k_delcap(uint64_t i)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_DELETE_CAP;
	register uint64_t a0 __asm__("a0") = i;
	__asm__ volatile("ecall" : "+r"(t0) : "r"(a0));
	return t0;
}

static inline s3k_excpt_t s3k_revcap(uint64_t i)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_REVOKE_CAP;
	register uint64_t a0 __asm__("a0") = i;
	__asm__ volatile("ecall" : "+r"(t0) : "r"(a0));
	return t0;
}

static inline s3k_excpt_t s3k_drvcap(uint64_t orig, uint64_t dest,
				     s3k_cap_t new_cap)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_DERIVE_CAP;
	register uint64_t a0 __asm__("a0") = orig;
	register uint64_t a1 __asm__("a1") = dest;
	register uint64_t a2 __asm__("a2") = new_cap.raw;
	__asm__ volatile("ecall" : "+r"(t0) : "r"(a0), "r"(a1), "r"(a2));
	return t0;
}

static inline s3k_excpt_t s3k_pmpset(uint64_t pmp_cidx, uint64_t pmp_idx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_PMP_SET;
	register uint64_t a0 __asm__("a0") = pmp_cidx;
	register uint64_t a1 __asm__("a1") = pmp_idx;
	__asm__ volatile("ecall" : "+r"(t0) : "r"(a0), "r"(a1));
	return t0;
}

static inline s3k_excpt_t s3k_pmpclear(uint64_t pmp_cidx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_PMP_CLEAR;
	register uint64_t a0 __asm__("a0") = pmp_cidx;
	__asm__ volatile("ecall" : "+r"(t0) : "r"(a0));
	return t0;
}

static inline s3k_excpt_t s3k_msuspend(uint64_t mon_cidx, uint64_t pid)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_SUSPEND;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	__asm__ volatile("ecall" : "+r"(t0) : "r"(a0), "r"(a1));
	return t0;
}

static inline s3k_excpt_t s3k_mresume(uint64_t mon_cidx, uint64_t pid)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_RESUME;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	__asm__ volatile("ecall" : "+r"(t0) : "r"(a0), "r"(a1));
	return t0;
}

static inline s3k_excpt_t s3k_mgetreg(uint64_t mon_cidx, uint64_t pid,
				      uint64_t reg, uint64_t *val)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_GET_REG;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = reg;
	__asm__ volatile("ecall" : "+r"(t0), "+r"(a0) : "r"(a1), "r"(a2));
	if (t0 == S3K_EXCPT_NONE)
		*val = a0;
	return t0;
}

static inline s3k_excpt_t s3k_msetreg(uint64_t mon_cidx, uint64_t pid,
				      uint64_t reg, uint64_t val)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_SET_REG;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = reg;
	register uint64_t a3 __asm__("a3") = val;
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0)
			 : "r"(a1), "r"(a2), "r"(a3));
	return t0;
}

static inline s3k_excpt_t s3k_mgetcap(uint64_t mon_cidx, uint64_t pid,
				      uint64_t cidx, s3k_cap_t *cap)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_GET_CAP;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = cidx;
	__asm__ volatile("ecall" : "+r"(t0), "+r"(a0) : "r"(a1), "r"(a2));
	if (t0 == S3K_EXCPT_NONE)
		cap->raw = a0;
	return t0;
}

static inline s3k_excpt_t s3k_mtakecap(uint64_t mon_cidx, uint64_t pid,
				       uint64_t src_cidx, uint64_t dest_cidx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_TAKE_CAP;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = src_cidx;
	register uint64_t a3 __asm__("a3") = dest_cidx;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1), "r"(a2), "r"(a3));
	return t0;
}

static inline s3k_excpt_t s3k_mgivecap(uint64_t mon_cidx, uint64_t pid,
				       uint64_t src_cidx, uint64_t dest_cidx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_GIVE_CAP;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = src_cidx;
	register uint64_t a3 __asm__("a3") = dest_cidx;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1), "r"(a2), "r"(a3));
	return t0;
}

static inline s3k_excpt_t s3k_mpmpset(uint64_t mon_cidx, uint64_t pid,
				      uint64_t pmp_cidx, uint64_t pmp_index)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_PMP_SET;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = pmp_cidx;
	register uint64_t a3 __asm__("a3") = pmp_index;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1), "r"(a2), "r"(a3));
	return t0;
}

static inline s3k_excpt_t s3k_mpmpclear(uint64_t mon_cidx, uint64_t pid,
					uint64_t pmp_cidx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_PMP_CLEAR;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = pmp_cidx;
	__asm__ volatile("ecall" : "+r"(t0) : "r"(a0), "r"(a1), "r"(a2));
	return t0;
}

static inline s3k_excpt_t s3k_send_cap(uint64_t sock_cidx, uint64_t buf[4],
				       uint64_t buf_cidx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_SOCKET_SEND;
	register uint64_t a0 __asm__("a0") = sock_cidx;
	register uint64_t a1 __asm__("a1") = buf[0];
	register uint64_t a2 __asm__("a2") = buf[1];
	register uint64_t a3 __asm__("a3") = buf[2];
	register uint64_t a4 __asm__("a4") = buf[3];
	register uint64_t a5 __asm__("a5") = buf_cidx;
	__asm__ volatile("ecall"
			 : "+r"(t0)
			 : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4),
			   "r"(a5));
	return t0;
}

static inline s3k_excpt_t s3k_send(uint64_t sock_cidx, uint64_t buf[4])
{
	return s3k_send_cap(sock_cidx, buf, -1);
}

static inline s3k_excpt_t s3k_recv_cap(uint64_t sock_cidx, uint64_t msgs[4],
				       uint64_t buf_cidx, s3k_cap_t *cap,
				       uint64_t *tag)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_SOCKET_RECV;
	register uint64_t a0 __asm__("a0") = sock_cidx;
	register uint64_t a1 __asm__("a1");
	register uint64_t a2 __asm__("a2");
	register uint64_t a3 __asm__("a3");
	register uint64_t a4 __asm__("a4");
	register uint64_t a5 __asm__("a5") = buf_cidx;
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0), "=r"(a1), "=r"(a2), "=r"(a3),
			   "=r"(a4), "=r"(a5));
	if (t0 == S3K_EXCPT_NONE) {
		*tag = a0;
		msgs[0] = a1;
		msgs[1] = a2;
		msgs[2] = a3;
		msgs[3] = a4;
		cap->raw = a5;
	}
	return t0;
}

static inline s3k_excpt_t s3k_recv(uint64_t sock_cidx, uint64_t buf[4],
				   uint64_t *tag)
{
	s3k_cap_t cap;
	return s3k_recv_cap(sock_cidx, buf, -1, &cap, tag);
}

static inline s3k_excpt_t s3k_sendrecv_cap(uint64_t sock_cidx, uint64_t buf[4],
					   uint64_t buf_cidx, s3k_cap_t *cap,
					   uint64_t *tag)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_SOCKET_SENDRECV;
	register uint64_t a0 __asm__("a0") = sock_cidx;
	register uint64_t a1 __asm__("a1") = buf[0];
	register uint64_t a2 __asm__("a2") = buf[1];
	register uint64_t a3 __asm__("a3") = buf[2];
	register uint64_t a4 __asm__("a4") = buf[3];
	register uint64_t a5 __asm__("a5") = buf_cidx;
	__asm__ volatile("ecall"
			 : "+r"(t0), "+r"(a0), "+r"(a1), "+r"(a2), "+r"(a3),
			   "+r"(a4), "+r"(a5));
	if (t0 == S3K_EXCPT_NONE) {
		*tag = a0;
		buf[0] = a1;
		buf[1] = a2;
		buf[2] = a3;
		buf[3] = a4;
		cap->raw = a5;
	}
	return t0;
}

static inline s3k_excpt_t s3k_sendrecv(uint64_t sock_cidx, uint64_t buf[4],
				       uint64_t *tag)
{
	s3k_cap_t cap;
	return s3k_sendrecv_cap(sock_cidx, buf, -1, &cap, tag);
}

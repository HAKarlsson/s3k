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
	S3K_R = 1,
	S3K_RW = 3,
	S3K_RX = 5,
	S3K_RWX = 7
} s3k_perm_t;

typedef enum {
	S3K_CAPTY_NULL,    ///< No capability.
	S3K_CAPTY_TIME,    ///< Time Slice capability.
	S3K_CAPTY_MEMORY,  ///< Memory Slice capability.
	S3K_CAPTY_PMP,     ///< PMP Frame capability.
	S3K_CAPTY_MONITOR, ///< Monitor capability.
	S3K_CAPTY_CHANNEL, ///< IPC Channel capability.
	S3K_CAPTY_SOCKET,  ///< IPC Socket capability.
	S3K_CAPTY_COUNT,   ///< Number of capabilitiy types.
} s3k_capty_t;

typedef enum {
	S3K_SYSCALL_GET_INFO,
	S3K_SYSCALL_GET_REG,
	S3K_SYSCALL_SET_REG,
	S3K_SYSCALL_YIELD,
	S3K_SYSCALL_GET_CAP,
	S3K_SYSCALL_MOVE_CAP,
	S3K_SYSCALL_DELETE_CAP,
	S3K_SYSCALL_REVOKE_CAP,
	S3K_SYSCALL_DERIVE_CAP,
	S3K_SYSCALL_PMP_SET,
	S3K_SYSCALL_PMP_CLEAR,
	S3K_SYSCALL_MONITOR_SUSPEND,
	S3K_SYSCALL_MONITOR_RESUME,
	S3K_SYSCALL_MONITOR_GET_REG,
	S3K_SYSCALL_MONITOR_SET_REG,
	S3K_SYSCALL_MONITOR_GET_CAP,
	S3K_SYSCALL_MONITOR_TAKE_CAP,
	S3K_SYSCALL_MONITOR_GIVE_CAP,
	S3K_SYSCALL_MONITOR_PMP_SET,
	S3K_SYSCALL_MONITOR_PMP_CLEAR,
	S3K_SYSCALL_SOCKET_SEND,
	S3K_SYSCALL_SOCKET_RECV,
	S3K_SYSCALL_SOCKET_SENDRECV,
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

typedef struct {
	uint64_t type : 4;
	uint64_t unused : 4;
	uint64_t hartid : 8;
	uint64_t base : 16;
	uint64_t alloc : 16;
	uint64_t size : 16;
} s3k_cap_time_t;

typedef struct {
	uint64_t type : 4;
	uint64_t rwx : 3;
	uint64_t lock : 1;
	uint64_t base : 24;
	uint64_t alloc : 16;
	uint64_t size : 16;
} s3k_cap_memory_t;

typedef struct {
	uint64_t type : 4;
	uint64_t rwx : 3;
	uint64_t used : 1;
	uint64_t index : 8;
	uint64_t addr : 48;
} s3k_cap_pmp_t;

typedef struct {
	uint64_t type : 4;
	uint64_t unused : 12;
	uint64_t base : 16;
	uint64_t alloc : 16;
	uint64_t size : 16;
} s3k_cap_monitor_t;

typedef struct {
	uint64_t type : 4;
	uint64_t unused : 12;
	uint64_t base : 16;
	uint64_t alloc : 16;
	uint64_t size : 16;
} s3k_cap_channel_t;

typedef struct {
	uint64_t type : 4;
	uint64_t unused : 28;
	uint64_t channel : 16;
	uint64_t tag : 16;
} s3k_cap_socket_t;

/// Capability description
typedef union {
	uint64_t type : 4;
	uint64_t raw;
	s3k_cap_time_t time;
	s3k_cap_memory_t mem;
	s3k_cap_pmp_t pmp;
	s3k_cap_monitor_t mon;
	s3k_cap_channel_t chan;
	s3k_cap_socket_t sock;
} s3k_cap_t;

// Utility functions
static inline void s3k_napot_decode(uint64_t addr, uint64_t *base, uint64_t *size)
{
	*base = ((addr + 1) & addr) << 2;
	*size =  (((addr + 1) ^ addr) + 1) << 2;
}

static inline uint64_t s3k_napot_encode(uint64_t base, uint64_t size)
{
	return (base | (size / 2 - 1)) >> 2;
}

static inline s3k_cap_t s3k_mktime(uint64_t hartid, uint64_t base, uint64_t size)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_TIME;
	cap.time.hartid = hartid;
	cap.time.base = base;
	cap.time.size = size;
	cap.time.alloc = 0;
	return cap;
}

static inline s3k_cap_t s3k_mkmemory(uint64_t base, uint64_t size, uint64_t rwx)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_MEMORY;
	cap.mem.base = base;
	cap.mem.size = size;
	cap.mem.rwx = rwx;
	cap.mem.lock = 0;
	return cap;
}

static inline s3k_cap_t s3k_mkpmp(uint64_t addr, uint64_t rwx)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_PMP;
	cap.pmp.addr = addr;
	cap.pmp.rwx = rwx;
	cap.pmp.used = 0;
	cap.pmp.index = 0;
	return cap;
}

static inline s3k_cap_t s3k_mkmonitor(uint64_t base, uint64_t size)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_MONITOR;
	cap.mon.base = base;
	cap.mon.size = size;
	return cap;
}

static inline s3k_cap_t s3k_mkchannel(uint64_t base, uint64_t size)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_CHANNEL;
	cap.chan.base = base;
	cap.chan.size = size;
	return cap;
}

static inline s3k_cap_t s3k_mksocket(uint64_t channel, uint64_t tag)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_SOCKET;
	cap.sock.channel = channel;
	cap.sock.tag = tag;
	return cap;
}

static inline int s3k_is_parent(s3k_cap_t a, s3k_cap_t b) {
	if (a.type == S3K_CAPTY_TIME && b.type == S3K_CAPTY_TIME) {
		return a.time.hartid && b.time.hartid 
			&& a.time.base <= b.time.base 
			&& (b.time.base + b.time.size) <= (a.time.base + a.time.size);
	}

	if (a.type == S3K_CAPTY_MEMORY && b.type == S3K_CAPTY_MEMORY) {
		return a.mem.base <= b.mem.base
			&& (b.mem.base + b.mem.size) <= (a.mem.base + a.mem.size)
			&& (a.mem.rwx & b.mem.rwx) == b.mem.rwx;
	}

	if (a.type == S3K_CAPTY_MEMORY && b.type == S3K_CAPTY_PMP) {
		uint64_t mem_base = a.mem.base << 12;
		uint64_t mem_size = a.mem.size << 12;
		uint64_t pmp_begin, pmp_end;
		s3k_napot_decode(b.pmp.addr, &pmp_begin, &pmp_end);
		return mem_base <= pmp_begin
			&& pmp_end <= (mem_base + mem_size)
			&& (a.mem.rwx & b.pmp.rwx) == b.pmp.rwx;
	}

	if (a.type == S3K_CAPTY_MONITOR && b.type == S3K_CAPTY_MONITOR) {
		return a.mon.base <= b.mon.base 
			&& (b.mon.base + b.mon.size) <= (a.mon.base + a.mon.size);
	}

	if (a.type == S3K_CAPTY_CHANNEL && b.type == S3K_CAPTY_CHANNEL) {
		return a.chan.base <= b.chan.base 
			&& (b.chan.base + b.chan.size) <= (a.chan.base + a.chan.size);
	}

	if (a.type == S3K_CAPTY_CHANNEL && b.type == S3K_CAPTY_SOCKET) {
		return a.chan.base <= b.sock.channel 
			&& b.sock.channel < (a.chan.base + a.chan.size);
	}

	if (a.type == S3K_CAPTY_SOCKET && b.type == S3K_CAPTY_SOCKET) {
		return a.sock.channel == b.sock.channel 
			&& a.sock.tag == 0 && b.sock.tag != 0;
	}

	return 0;
}

static inline int s3k_is_derivable(s3k_cap_t a, s3k_cap_t b) {
	if (a.type == S3K_CAPTY_TIME && b.type == S3K_CAPTY_TIME) {
		return a.time.hartid && b.time.hartid 
			&& (a.time.base + a.time.size) == b.time.base 
			&& (b.time.base + b.time.size) <= (a.time.base + a.time.size);
	}

	if (a.type == S3K_CAPTY_MEMORY && b.type == S3K_CAPTY_MEMORY) {
		return (a.mem.base + a.time.size) == b.mem.base
			&& (b.mem.base + b.mem.size) <= (a.mem.base + a.mem.size)
			&& (a.mem.rwx & b.mem.rwx) == b.mem.rwx;
	}

	if (a.type == S3K_CAPTY_MEMORY && b.type == S3K_CAPTY_PMP) {
		uint64_t mem_base = a.mem.base << 12;
		uint64_t mem_size = a.mem.size << 12;
		uint64_t pmp_begin, pmp_end;
		s3k_napot_decode(b.pmp.addr, &pmp_begin, &pmp_end);
		return mem_base <= pmp_begin
			&& pmp_end <= (mem_base + mem_size)
			&& (a.mem.rwx & b.pmp.rwx) == b.pmp.rwx;
	}

	if (a.type == S3K_CAPTY_MONITOR && b.type == S3K_CAPTY_MONITOR) {
		return (a.mon.base + a.mon.alloc) <= b.mon.base 
			&& (b.mon.base + b.mon.size) <= (a.mon.base + a.mon.size);
	}

	if (a.type == S3K_CAPTY_CHANNEL && b.type == S3K_CAPTY_CHANNEL) {
		return (a.chan.base + a.chan.alloc) <= b.chan.base 
			&& (b.chan.base + b.chan.size) <= (a.chan.base + a.chan.size);
	}

	if (a.type == S3K_CAPTY_CHANNEL && b.type == S3K_CAPTY_SOCKET) {
		return (a.chan.base + a.chan.alloc) <= b.sock.channel 
			&& b.sock.channel < (a.chan.base + a.chan.size);
	}

	if (a.type == S3K_CAPTY_SOCKET && b.type == S3K_CAPTY_SOCKET) {
		return a.sock.channel == b.sock.channel 
			&& a.sock.tag == 0 && b.sock.tag != 0;
	}

	return 0;
}

// System calls
static inline uint64_t s3k_getpid(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_INFO;
	register uint64_t a0 __asm__("a0") = 0;
	__asm__ volatile ("ecall" : "+r"(t0), "+r"(a0));
	return a0;
}

static inline uint64_t s3k_gettime(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_INFO;
	register uint64_t a0 __asm__("a0") = 1;
	__asm__ volatile ("ecall" : "+r"(t0), "+r"(a0));
	return a0;
}

static inline uint64_t s3k_gettimeout(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_INFO;
	register uint64_t a0 __asm__("a0") = 2;
	__asm__ volatile ("ecall" : "+r"(t0), "+r"(a0));
	return a0;
}

static inline uint64_t s3k_getwcet(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_INFO;
	register uint64_t a0 __asm__("a0") = 3;
	__asm__ volatile ("ecall" : "+r"(t0), "+r"(a0));
	return a0;
}

static inline s3k_excpt_t s3k_getreg(uint64_t reg, uint64_t *val)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_REG;
	register uint64_t a0 __asm__("a0") = reg;
	__asm__ volatile ("ecall" : "+r"(t0), "+r"(a0));
	*val = a0;
	return t0;
}

static inline s3k_excpt_t s3k_setreg(uint64_t reg, uint64_t val)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_SET_REG;
	register uint64_t a0 __asm__("a0") = reg;
	register uint64_t a1 __asm__("a1") = val;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0), "r"(a1));
	return t0;
}

static inline void s3k_yield(void)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_YIELD;
	register uint64_t a0 __asm__("a0") = 0;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0));
}

static inline s3k_excpt_t s3k_getcap(uint64_t i, s3k_cap_t *cap)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_GET_CAP;
	register uint64_t a0 __asm__("a0") = i;
	__asm__ volatile ("ecall" : "+r"(t0), "+r"(a0));
	if (t0 == S3K_EXCPT_NONE)
		cap->raw = a0;
	return t0;
}

static inline s3k_excpt_t s3k_movcap(uint64_t src, uint64_t dest)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MOVE_CAP;
	register uint64_t a0 __asm__("a0") = src;
	register uint64_t a1 __asm__("a1") = dest;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0), "r"(a1));
	return t0;
}

static inline s3k_excpt_t s3k_delcap(uint64_t i)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_DELETE_CAP;
	register uint64_t a0 __asm__("a0") = i;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0));
	return t0;
}

static inline s3k_excpt_t s3k_revcap(uint64_t i)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_REVOKE_CAP;
	register uint64_t a0 __asm__("a0") = i;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0));
	return t0;
}

static inline s3k_excpt_t s3k_drvcap(uint64_t orig, uint64_t dest, s3k_cap_t new_cap)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_DERIVE_CAP;
	register uint64_t a0 __asm__("a0") = orig;
	register uint64_t a1 __asm__("a1") = dest;
	register uint64_t a2 __asm__("a2") = new_cap.raw;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0), "r"(a1), "r"(a2));
	return t0;
}

static inline s3k_excpt_t s3k_pmpset(uint64_t pmp_cidx, uint64_t pmp_idx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_PMP_SET;
	register uint64_t a0 __asm__("a0") = pmp_cidx;
	register uint64_t a1 __asm__("a1") = pmp_idx;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0), "r"(a1));
	return t0;
}

static inline s3k_excpt_t s3k_pmpclear(uint64_t pmp_cidx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_PMP_CLEAR;
	register uint64_t a0 __asm__("a0") = pmp_cidx;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0));
	return t0;
}

static inline s3k_excpt_t s3k_msuspend(uint64_t mon_cidx, uint64_t pid)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_SUSPEND;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0), "r"(a1));
	return t0;
}

static inline s3k_excpt_t s3k_mresume(uint64_t mon_cidx, uint64_t pid)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_RESUME;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0), "r"(a1));
	return t0;
}

static inline s3k_excpt_t s3k_mgetreg(uint64_t mon_cidx, uint64_t pid, uint64_t reg, uint64_t *val)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_GET_REG;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = reg;
	__asm__ volatile ("ecall" : "+r"(t0), "+r"(a0) : "r"(a1), "r"(a2));
	if (t0 == S3K_EXCPT_NONE)
		*val = a0;
	return t0;
}

static inline s3k_excpt_t s3k_msetreg(uint64_t mon_cidx, uint64_t pid, uint64_t reg, uint64_t val)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_SET_REG;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = reg;
	register uint64_t a3 __asm__("a3") = val;
	__asm__ volatile ("ecall" : "+r"(t0), "+r"(a0) : "r"(a1), "r"(a2), "r"(a3));
	return t0;
}

static inline s3k_excpt_t s3k_mgetcap(uint64_t mon_cidx, uint64_t pid, uint64_t cidx, s3k_cap_t *cap)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_GET_CAP;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = cidx;
	__asm__ volatile ("ecall" : "+r"(t0), "+r"(a0) : "r"(a1), "r"(a2));
	if (t0 == S3K_EXCPT_NONE)
		cap->raw = a0;
	return t0;
}

static inline s3k_excpt_t s3k_mtakecap(uint64_t mon_cidx, uint64_t pid, uint64_t src_cidx, uint64_t dest_cidx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_TAKE_CAP;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = src_cidx;
	register uint64_t a3 __asm__("a3") = dest_cidx;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0), "r"(a1), "r"(a2), "r"(a3));
	return t0;
}

static inline s3k_excpt_t s3k_mgivecap(uint64_t mon_cidx, uint64_t pid, uint64_t src_cidx, uint64_t dest_cidx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_GIVE_CAP;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = src_cidx;
	register uint64_t a3 __asm__("a3") = dest_cidx;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0), "r"(a1), "r"(a2), "r"(a3));
	return t0;
}

static inline s3k_excpt_t s3k_mpmpset(uint64_t mon_cidx, uint64_t pid, uint64_t pmp_cidx, uint64_t pmp_index)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_PMP_SET;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = pmp_cidx;
	register uint64_t a3 __asm__("a3") = pmp_index;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0), "r"(a1), "r"(a2), "r"(a3));
	return t0;
}

static inline s3k_excpt_t s3k_mpmpclear(uint64_t mon_cidx, uint64_t pid, uint64_t pmp_cidx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_MONITOR_PMP_CLEAR;
	register uint64_t a0 __asm__("a0") = mon_cidx;
	register uint64_t a1 __asm__("a1") = pid;
	register uint64_t a2 __asm__("a2") = pmp_cidx;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0), "r"(a1), "r"(a2));
	return t0;
}

static inline s3k_excpt_t s3k_send_cap(uint64_t sock_cidx, uint64_t buf[4], uint64_t buf_cidx)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_SOCKET_SEND;
	register uint64_t a0 __asm__("a0") = sock_cidx;
	register uint64_t a1 __asm__("a1") = buf[0];
	register uint64_t a2 __asm__("a2") = buf[1];
	register uint64_t a3 __asm__("a3") = buf[2];
	register uint64_t a4 __asm__("a4") = buf[3];
	register uint64_t a5 __asm__("a5") = buf_cidx;
	__asm__ volatile ("ecall" : "+r"(t0) : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5));
	return t0;
}

static inline s3k_excpt_t s3k_send(uint64_t sock_cidx, uint64_t buf[4])
{
	return s3k_send_cap(sock_cidx, buf, -1);
}

static inline s3k_excpt_t s3k_recv_cap(uint64_t sock_cidx, uint64_t msgs[4], 
		uint64_t buf_cidx, s3k_cap_t *cap, uint64_t *tag)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_SOCKET_RECV;
	register uint64_t a0 __asm__("a0") = sock_cidx;
	register uint64_t a1 __asm__("a1");
	register uint64_t a2 __asm__("a2");
	register uint64_t a3 __asm__("a3");
	register uint64_t a4 __asm__("a4");
	register uint64_t a5 __asm__("a5") = buf_cidx;
	__asm__ volatile ("ecall" : "+r"(t0), "+r"(a0), "=r"(a1), "=r"(a2), "=r"(a3), "=r"(a4), "=r"(a5));
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

static inline s3k_excpt_t s3k_recv(uint64_t sock_cidx, uint64_t buf[4], uint64_t *tag)
{
	s3k_cap_t cap;
	return s3k_recv_cap(sock_cidx, buf, -1, &cap, tag);
}


static inline s3k_excpt_t s3k_sendrecv_cap(uint64_t sock_cidx, uint64_t buf[4], 
		uint64_t buf_cidx, s3k_cap_t *cap, uint64_t *tag)
{
	register uint64_t t0 __asm__("t0") = S3K_SYSCALL_SOCKET_SENDRECV;
	register uint64_t a0 __asm__("a0") = sock_cidx;
	register uint64_t a1 __asm__("a1") = buf[0];
	register uint64_t a2 __asm__("a2") = buf[1];
	register uint64_t a3 __asm__("a3") = buf[2];
	register uint64_t a4 __asm__("a4") = buf[3];
	register uint64_t a5 __asm__("a5") = buf_cidx;
	__asm__ volatile ("ecall" : "+r"(t0), "+r"(a0), "+r"(a1), "+r"(a2), "+r"(a3), "+r"(a4), "+r"(a5));
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

static inline s3k_excpt_t s3k_sendrecv(uint64_t sock_cidx, uint64_t buf[4], uint64_t *tag)
{
	s3k_cap_t cap;
	return s3k_sendrecv_cap(sock_cidx, buf, -1, &cap, tag);
}

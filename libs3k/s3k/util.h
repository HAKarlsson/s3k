#ifndef S3K_UTIL_H
#define S3K_UTIL_H
#include "cap.h"

#include <stdint.h>

// Utility functions
static inline void s3k_napot_decode(uint64_t addr, uint64_t *base,
				    uint64_t *size)
{
	*base = ((addr + 1) & addr) << 2;
	*size = (((addr + 1) ^ addr) + 1) << 2;
}

static inline uint64_t s3k_napot_encode(uint64_t base, uint64_t size)
{
	return (base | (size / 2 - 1)) >> 2;
}

static inline int s3k_is_parent(s3k_cap_t a, s3k_cap_t b)
{
	if (a.type == S3K_CAPTY_TIME && b.type == S3K_CAPTY_TIME) {
		return a.time.hart == b.time.hart && a.time.base <= b.time.base
		       && (b.time.base + b.time.size)
			      <= (a.time.base + a.time.size);
	}

	if (a.type == S3K_CAPTY_MEMORY && b.type == S3K_CAPTY_MEMORY) {
		return a.memory.base <= b.memory.base
		       && (b.memory.base + b.memory.size)
			      <= (a.memory.base + a.memory.size)
		       && (a.memory.rwx & b.memory.rwx) == b.memory.rwx;
	}

	if (a.type == S3K_CAPTY_MEMORY && b.type == S3K_CAPTY_PMP) {
		uint64_t mem_base = a.memory.base << 12;
		uint64_t mem_size = a.memory.size << 12;
		uint64_t pmp_begin, pmp_end;
		s3k_napot_decode(b.pmp.addr, &pmp_begin, &pmp_end);
		return mem_base <= pmp_begin && pmp_end <= (mem_base + mem_size)
		       && (a.memory.rwx & b.pmp.rwx) == b.pmp.rwx;
	}

	if (a.type == S3K_CAPTY_MONITOR && b.type == S3K_CAPTY_MONITOR) {
		return a.monitor.base <= b.monitor.base
		       && (b.monitor.base + b.monitor.size)
			      <= (a.monitor.base + a.monitor.size);
	}

	if (a.type == S3K_CAPTY_CHANNEL && b.type == S3K_CAPTY_CHANNEL) {
		return a.channel.base <= b.channel.base
		       && (b.channel.base + b.channel.size)
			      <= (a.channel.base + a.channel.size);
	}

	if (a.type == S3K_CAPTY_CHANNEL && b.type == S3K_CAPTY_SOCKET) {
		return a.channel.base <= b.socket.channel
		       && b.socket.channel < (a.channel.base + a.channel.size);
	}

	if (a.type == S3K_CAPTY_SOCKET && b.type == S3K_CAPTY_SOCKET) {
		return a.socket.channel == b.socket.channel
		       && a.socket.mode == b.socket.mode
		       && a.socket.perm == b.socket.perm && a.socket.tag == 0
		       && b.socket.tag != 0;
	}

	return 0;
}

static inline int s3k_is_derivable(s3k_cap_t a, s3k_cap_t b)
{
	if (a.type == S3K_CAPTY_TIME && b.type == S3K_CAPTY_TIME) {
		return a.time.hart == b.time.hart
		       && (a.time.base + a.time.alloc) == b.time.base
		       && (b.time.base + b.time.size)
			      <= (a.time.base + a.time.size);
	}

	if (a.type == S3K_CAPTY_MEMORY && b.type == S3K_CAPTY_MEMORY) {
		return (a.memory.base + a.time.alloc) == b.memory.base
		       && (b.memory.base + b.memory.size)
			      <= (a.memory.base + a.memory.size)
		       && a.memory.lock == 0 && b.memory.lock == 0
		       && (a.memory.rwx & b.memory.rwx) == b.memory.rwx;
	}

	if (a.type == S3K_CAPTY_MEMORY && b.type == S3K_CAPTY_PMP) {
		uint64_t memory_base = a.memory.base << 12;
		uint64_t memory_alloc = a.memory.alloc << 12;
		uint64_t memory_size = a.memory.size << 12;
		uint64_t pmp_begin, pmp_end;
		s3k_napot_decode(b.pmp.addr, &pmp_begin, &pmp_end);
		return (memory_base + memory_alloc) <= pmp_begin
		       && pmp_end <= (memory_base + memory_size)
		       && (a.memory.rwx & b.pmp.rwx) == b.pmp.rwx;
	}

	if (a.type == S3K_CAPTY_MONITOR && b.type == S3K_CAPTY_MONITOR) {
		return (a.monitor.base + a.monitor.alloc) == b.monitor.base
		       && (b.monitor.base + b.monitor.size)
			      <= (a.monitor.base + a.monitor.size);
	}

	if (a.type == S3K_CAPTY_CHANNEL && b.type == S3K_CAPTY_CHANNEL) {
		return (a.channel.base + a.channel.alloc) == b.channel.base
		       && (b.channel.base + b.channel.size)
			      <= (a.channel.base + a.channel.size);
	}

	if (a.type == S3K_CAPTY_CHANNEL && b.type == S3K_CAPTY_SOCKET) {
		return (a.channel.base + a.channel.alloc) == b.socket.channel
		       && b.socket.channel < (a.channel.base + a.channel.size)
		       && b.socket.tag == 0;
	}

	if (a.type == S3K_CAPTY_SOCKET && b.type == S3K_CAPTY_SOCKET) {
		return a.socket.channel == b.socket.channel
		       && a.socket.mode == b.socket.mode
		       && a.socket.perm == b.socket.perm && a.socket.tag == 0
		       && b.socket.tag != 0;
	}

	return 0;
}

#endif /* S3K_UTIL_H */

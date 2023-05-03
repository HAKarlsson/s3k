/**
 * @file cap.h
 * @brief Functions for handling capabilities.
 * @copyright MIT License
 * @author Henrik Karlsson (henrik10@kth.se)
 */
#ifndef __CAP_H__
#define __CAP_H__

#include "common.h"
#include "consts.h"
#include "pmp.h"

#include <stdbool.h>
#include <stdint.h>

#define CAP_RWX 0x7
#define CAP_RW 0x3
#define CAP_RX 0x5
#define CAP_R 0x1

/* clang-format off */
#define CAP_NONE ((cap_t){ .raw = 0 })

#define CAP_TIME(_hartid, _begin, _end)      \
	((cap_t){                            \
		.time = {.type = CAPTY_TIME, \
		         .unused = 0,        \
		         .hartid = _hartid,  \
		         .begin = _begin,    \
		         .free = _begin,     \
		         .end = _end }       \
	})

#define CAP_MEMORY(_begin, _end, _offset, _rwx) \
	((cap_t){                               \
	    .memory = {.type = CAPTY_MEMORY,    \
		       .rwx = _rwx,             \
		       .lock = 0,               \
		       .offset = _offset,       \
		       .begin = _begin,         \
		       .free = _begin,          \
		       .end = _end}})

#define CAP_PMP(_addr, _cfg)            \
	((cap_t){                       \
            .pmp = {.type = CAPTY_PMP,  \
                    .addr = _addr,      \
                    .idx = 0,      \
                    .used = 0,     \
                    .rwx = _cfg} \
        })

#define CAP_MONITOR(_begin, _end)              \
	((cap_t){                              \
	    .monitor = {.type = CAPTY_MONITOR, \
			.unused = 0,           \
			.begin = _begin,       \
			.free = _begin,        \
			.end = _end}           \
        })

#define CAP_CHANNEL(_begin, _end)             \
	((cap_t){                             \
		.channel                      \
		    = {.type = CAPTY_CHANNEL, \
		       .unused = 0,           \
		       .begin = _begin,       \
		       .free = _begin,        \
		       .end = _end }          \
	})

#define CAP_SOCKET(_channel, _tag)           \
	((cap_t){                            \
		.socket                      \
		    = {.type = CAPTY_SOCKET, \
		       .unused = 0,          \
		       .channel = _channel,  \
		       .tag = _tag,          \
                       .listeners = 0}       \
	})

/* clang-format on */

typedef enum {
	CAPTY_NONE,    ///< No capability
	CAPTY_TIME,    ///< Time Slice capability
	CAPTY_MEMORY,  ///< Memory Slice capability
	CAPTY_PMP,     ///< PMP Frame capability
	CAPTY_MONITOR, ///< Monitor capability
	CAPTY_CHANNEL, ///< IPC Channel capability
	CAPTY_SOCKET,  ///< IPC Socket capability
} capty_t;

/// Capability description
typedef union {
	uint64_t type : 4;
	uint64_t raw;

	struct {
		uint64_t type : 4;
		uint64_t unused : 4;
		uint64_t hartid : 8;
		uint64_t begin : 16;
		uint64_t free : 16;
		uint64_t end : 16;
	} time;

	struct {
		uint64_t type : 4;
		uint64_t rwx : 3;
		uint64_t lock : 1;
		uint64_t offset : 8;
		uint64_t begin : 16;
		uint64_t free : 16;
		uint64_t end : 16;
	} memory;

	struct {
		uint64_t type : 4;
		uint64_t rwx : 3;
		uint64_t used : 1;
		uint64_t idx : 4;
		uint64_t addr : 48;
	} pmp;

	struct {
		uint64_t type : 4;
		uint64_t unused : 12;
		uint64_t begin : 16;
		uint64_t free : 16;
		uint64_t end : 16;
	} monitor;

	struct {
		uint64_t type : 4;
		uint64_t unused : 12;
		uint64_t begin : 16;
		uint64_t free : 16;
		uint64_t end : 16;
	} channel;

	struct {
		uint64_t type : 4;
		uint64_t unused : 12;
		uint64_t channel : 16;
		uint64_t tag : 16;
		uint64_t listerners : 16;
	} socket;
} cap_t;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(cap_t) == 8, "cap_t size != 8 bytes");
#endif

static inline uint64_t cap_get_type(cap_t cap)
{
	return cap.raw & 0xF;
}

static inline uint64_t cap_time_get_hartid(cap_t cap)
{
	return (cap.raw >> 8) & 0xFF;
}

static inline uint64_t cap_time_get_begin(cap_t cap)
{
	return (cap.raw >> 16) & 0xFFFF;
}

static inline uint64_t cap_time_get_end(cap_t cap)
{
	return (cap.raw >> 32) & 0xFFFF;
}

static inline uint64_t cap_time_get_free(cap_t cap)
{
	return (cap.raw >> 48) & 0xFFFF;
}

static inline void cap_time_set_free(cap_t *cap, uint64_t free)
{
	cap->raw &= ~0xFFFFull << 48;
	cap->raw |= free << 48;
}

static inline uint64_t cap_memory_get_offset(cap_t cap)
{
	return (cap.raw >> 8) & 0xFFull;
}

static inline uint64_t cap_memory_get_begin(cap_t cap)
{
	return (cap.raw >> 16) & 0xFFFF;
}

static inline uint64_t cap_memory_get_end(cap_t cap)
{
	return (cap.raw >> 32) & 0xFFFF;
}

static inline uint64_t cap_memory_get_free(cap_t cap)
{
	return (cap.raw >> 48) & 0xFFFF;
}

static inline uint64_t cap_memory_get_rwx(cap_t cap)
{
	return (cap.raw >> 4) & 0x7;
}

static inline uint64_t cap_memory_get_lock(cap_t cap)
{
	return (cap.raw >> 7) & 0x1;
}

static inline void cap_memory_set_free(cap_t *cap, uint64_t free)
{
	cap->raw &= ~0xFFFFull << 48;
	cap->raw |= free << 48;
}

static inline void cap_memory_set_lock(cap_t *cap, uint64_t lock)
{
	cap->raw &= ~1ull << 7;
	cap->raw |= lock << 7;
}

static inline uint64_t cap_pmp_get_rwx(cap_t cap)
{
	return (cap.raw >> 4) & 0x7ull;
}

static inline uint64_t cap_pmp_get_used(cap_t cap)
{
	return (cap.raw >> 7) & 0x1ull;
}

static inline uint64_t cap_pmp_get_idx(cap_t cap)
{
	return (cap.raw >> 8) & 0xFull;
}

static inline uint64_t cap_pmp_get_addr(cap_t cap)
{
	return cap.raw >> 12;
}

static inline void cap_pmp_set_used(cap_t *cap, uint64_t used)
{
	cap->raw &= ~1ull << 7;
	cap->raw |= used << 7;
}

static inline void cap_pmp_set_idx(cap_t *cap, uint64_t idx)
{
	cap->raw &= ~0xFull << 8;
	cap->raw |= idx << 8;
}

static inline uint64_t cap_monitor_get_begin(cap_t cap)
{
	return (cap.raw >> 16) & 0xFFFFull;
}

static inline uint64_t cap_monitor_get_end(cap_t cap)
{
	return (cap.raw >> 32) & 0xFFFFull;
}

static inline uint64_t cap_monitor_get_free(cap_t cap)
{
	return (cap.raw >> 48) & 0xFFFFull;
}

static inline void cap_monitor_set_free(cap_t *cap, uint64_t free)
{
	cap->raw &= ~0xFFFFull << 48;
	cap->raw |= free << 48;
}

static inline uint64_t cap_channel_get_begin(cap_t cap)
{
	return (cap.raw >> 16) & 0xFFFFull;
}

static inline uint64_t cap_channel_get_end(cap_t cap)
{
	return (cap.raw >> 32) & 0xFFFFull;
}

static inline uint64_t cap_channel_get_free(cap_t cap)
{
	return (cap.raw >> 48) & 0xFFFFull;
}

static inline void cap_channel_set_free(cap_t *cap, uint64_t free)
{
	cap->raw &= ~0xFFFFull << 48;
	cap->raw |= free << 48;
}

static inline uint64_t cap_socket_get_channel(cap_t cap)
{
	return (cap.raw >> 16) & 0xFFFFull;
}

static inline uint64_t cap_socket_get_tag(cap_t cap)
{
	return (cap.raw >> 32) & 0xFFFFull;
}

static inline uint64_t cap_socket_get_listeners(cap_t cap)
{
	return (cap.raw >> 48) & 0xFFFFull;
}

static inline void cap_socket_set_listeners(cap_t *cap, uint64_t listeners)
{
	cap->raw &= ~0xFFFFull << 48;
	cap->raw |= listeners << 48;
}

static inline bool cap_time_is_child(cap_t parent, cap_t child)
{
	if (cap_get_type(child) == CAPTY_TIME) {
		uint64_t parent_hartid = cap_time_get_hartid(parent);
		uint64_t parent_begin = cap_time_get_begin(parent);
		uint64_t parent_end = cap_time_get_end(parent);
		uint64_t child_hartid = cap_time_get_hartid(child);
		uint64_t child_begin = cap_time_get_begin(child);
		uint64_t child_end = cap_time_get_end(parent);
		return parent_hartid == child_hartid
		       && parent_begin <= child_begin
		       && child_end <= parent_end;
	}
	return false;
}

static inline bool cap_memory_is_child(cap_t parent, cap_t child)
{
	if (cap_get_type(child) == CAPTY_MEMORY) {
		uint64_t parent_offset = cap_memory_get_offset(parent);
		uint64_t parent_begin = cap_memory_get_begin(parent);
		uint64_t parent_end = cap_memory_get_end(parent);
		uint64_t child_offset = cap_memory_get_offset(child);
		uint64_t child_begin = cap_memory_get_begin(child);
		uint64_t child_end = cap_memory_get_end(child);
		return child_offset == parent_offset
		       && parent_begin <= child_begin
		       && child_end <= parent_end;
	}
	if (cap_get_type(child) == CAPTY_PMP) {
		uint64_t parent_begin = (cap_memory_get_offset(parent) << 27)
					+ (cap_memory_get_begin(parent) << 12);
		uint64_t parent_end = (cap_memory_get_offset(parent) << 27)
				      + (cap_memory_get_end(parent) << 12);
		uint64_t child_addr = cap_pmp_get_addr(child);
		uint64_t child_begin = pmp_napot_begin(child_addr);
		uint64_t child_end = pmp_napot_end(child_addr);
		return parent_begin <= child_begin && child_end <= parent_end;
	}
	return false;
}

static inline bool cap_channel_is_child(cap_t parent, cap_t child)
{
	if (cap_get_type(child) == CAPTY_CHANNEL) {
		uint64_t parent_begin = cap_channel_get_begin(parent);
		uint64_t parent_end = cap_channel_get_end(parent);
		uint64_t child_begin = cap_channel_get_begin(child);
		uint64_t child_end = cap_channel_get_end(child);
		return parent_begin <= child_begin && child_end <= parent_end;
	}
	if (cap_get_type(child) == CAPTY_SOCKET) {
		uint64_t parent_begin = cap_channel_get_begin(parent);
		uint64_t parent_end = cap_channel_get_end(parent);
		uint64_t child_channel = cap_socket_get_channel(child);
		return parent_begin <= child_channel
		       && child_channel <= parent_end;
	}
	return false;
}

static inline bool cap_socket_is_child(cap_t parent, cap_t child)
{
	if (cap_get_type(child) == CAPTY_SOCKET) {
		uint64_t parent_tag = cap_socket_get_tag(parent);
		uint64_t parent_channel = cap_socket_get_channel(parent);
		uint64_t child_channel = cap_socket_get_channel(child);
		return parent_tag == 0 && parent_channel == child_channel;
	}
	return false;
}

static inline bool cap_monitor_is_child(cap_t parent, cap_t child)
{
	if (cap_get_type(child) == CAPTY_MONITOR) {
		uint64_t parent_begin = cap_monitor_get_begin(parent);
		uint64_t parent_end = cap_monitor_get_end(parent);
		uint64_t child_begin = cap_monitor_get_begin(child);
		uint64_t child_end = cap_monitor_get_end(parent);
		return parent_begin <= child_begin && child_end <= parent_end;
	}

	return false;
}

static inline bool cap_is_child(cap_t parent, cap_t child)
{
	switch (cap_get_type(parent)) {
	case CAPTY_TIME:
		return cap_time_is_child(parent, child);
	case CAPTY_MEMORY:
		return cap_memory_is_child(parent, child);
	case CAPTY_MONITOR:
		return cap_monitor_is_child(parent, child);
	case CAPTY_CHANNEL:
		return cap_channel_is_child(parent, child);
	case CAPTY_SOCKET:
		return cap_socket_is_child(parent, child);
	default:
		return false;
	}
}

static inline bool cap_time_can_derive(cap_t parent, cap_t child)
{
	if (cap_get_type(child) == CAPTY_TIME) {
		uint64_t parent_hartid = cap_time_get_hartid(parent);
		uint64_t parent_free = cap_time_get_free(parent);
		uint64_t parent_end = cap_time_get_end(parent);
		uint64_t child_hartid = cap_time_get_hartid(child);
		uint64_t child_begin = cap_time_get_begin(child);
		uint64_t child_free = cap_time_get_free(child);
		uint64_t child_end = cap_time_get_end(child);
		return parent_hartid == child_hartid
		       && parent_free == child_begin && child_end <= parent_end
		       && child_begin == child_free
		       && child_begin <= parent_end;
	}
	return false;
}

static inline bool cap_memory_can_derive(cap_t parent, cap_t child)
{
	if (cap_get_type(child) == CAPTY_MEMORY) {
		uint64_t parent_offset = cap_memory_get_offset(parent);
		uint64_t parent_free = cap_memory_get_free(parent);
		uint64_t parent_end = cap_memory_get_end(parent);
		uint64_t parent_rwx = cap_memory_get_rwx(parent);
		uint64_t parent_lock = cap_memory_get_lock(parent);
		uint64_t child_offset = cap_memory_get_offset(parent);
		uint64_t child_begin = cap_memory_get_begin(child);
		uint64_t child_free = cap_memory_get_free(child);
		uint64_t child_end = cap_memory_get_end(child);
		uint64_t child_rwx = cap_memory_get_rwx(child);
		return parent_offset == child_offset
		       && parent_free == child_begin && child_end <= parent_end
		       && child_begin == child_free && child_begin <= parent_end
		       && (child_rwx & parent_rwx) == child_rwx
		       && parent_lock == 0;
	}
	if (cap_get_type(child) == CAPTY_PMP) {
		uint64_t parent_free = (cap_memory_get_offset(parent) << 27)
				       + (cap_memory_get_free(parent) << 12);
		uint64_t parent_end = (cap_memory_get_offset(parent) << 27)
				      + (cap_memory_get_end(parent) << 12);
		uint64_t parent_rwx = cap_memory_get_rwx(parent);
		uint64_t child_addr = cap_pmp_get_addr(child);
		uint64_t child_rwx = cap_pmp_get_rwx(child);
		uint64_t child_begin = pmp_napot_begin(child_addr);
		uint64_t child_end = pmp_napot_end(child_addr);
		return parent_free <= child_begin && child_end <= parent_end
		       && (child_rwx & parent_rwx) == child_rwx;
	}
	return false;
}

static inline bool cap_monitor_can_derive(cap_t parent, cap_t child)
{
	if (cap_get_type(child) == CAPTY_MONITOR) {
		uint64_t parent_free = cap_monitor_get_free(parent);
		uint64_t parent_end = cap_monitor_get_end(parent);
		uint64_t child_begin = cap_monitor_get_begin(child);
		uint64_t child_free = cap_monitor_get_free(child);
		uint64_t child_end = cap_monitor_get_end(child);
		return parent_free == child_begin && child_end <= parent_end
		       && child_begin == child_free
		       && child_begin <= parent_end;
	}
	return false;
}

static inline bool cap_channel_can_derive(cap_t parent, cap_t child)
{
	if (cap_get_type(child) == CAPTY_CHANNEL) {
		uint64_t parent_free = cap_channel_get_free(parent);
		uint64_t parent_end = cap_channel_get_end(parent);
		uint64_t child_begin = cap_channel_get_begin(child);
		uint64_t child_free = cap_channel_get_free(child);
		uint64_t child_end = cap_channel_get_end(child);
		return parent_free == child_begin && child_end <= parent_end
		       && child_begin == child_free
		       && child_begin <= parent_end;
	}
	if (cap_get_type(child) == CAPTY_SOCKET) {
		uint64_t parent_free = cap_channel_get_free(parent);
		uint64_t parent_end = cap_channel_get_end(parent);
		uint64_t child_channel = cap_socket_get_channel(child);
		uint64_t child_listeners = cap_socket_get_listeners(child);
		uint64_t child_tag = cap_socket_get_tag(child);
		return parent_free == child_channel
		       && child_channel <= parent_end && child_listeners == 0
		       && child_tag == 0;
	}
	return false;
}

static inline bool cap_socket_can_derive(cap_t parent, cap_t child)
{
	if (cap_get_type(child) == CAPTY_SOCKET) {
		uint64_t parent_listeners = cap_socket_get_listeners(parent);
		uint64_t parent_channel = cap_socket_get_channel(parent);
		uint64_t parent_tag = cap_socket_get_tag(parent);
		uint64_t child_listeners = cap_socket_get_listeners(child);
		uint64_t child_channel = cap_socket_get_channel(child);
		uint64_t child_tag = cap_socket_get_tag(child);
		return parent_channel == child_channel && parent_tag == 0
		       && parent_listeners < 0xFFFF && child_listeners == 0
		       && 0 < child_tag;
	}
	return false;
}

static inline bool cap_can_derive(cap_t parent, cap_t child)
{
	switch (cap_get_type(parent)) {
	case CAPTY_TIME:
		return cap_time_can_derive(parent, child);
	case CAPTY_MEMORY:
		return cap_memory_can_derive(parent, child);
	case CAPTY_MONITOR:
		return cap_monitor_can_derive(parent, child);
	case CAPTY_CHANNEL:
		return cap_channel_can_derive(parent, child);
	case CAPTY_SOCKET:
		return cap_socket_can_derive(parent, child);
	default:
		return false;
	}
}

#endif /* __CAP_H__ */

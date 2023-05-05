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
	CAPTY_NONE,    ///< No capability.
	CAPTY_TIME,    ///< Time Slice capability.
	CAPTY_MEMORY,  ///< Memory Slice capability.
	CAPTY_PMP,     ///< PMP Frame capability.
	CAPTY_MONITOR, ///< Monitor capability.
	CAPTY_CHANNEL, ///< IPC Channel capability.
	CAPTY_SOCKET,  ///< IPC Socket capability.
	CAPTY_COUNT,   ///< Number of capabilitiy types.
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
		uint64_t unused : 28;
		uint64_t channel : 16;
		uint64_t tag : 16;
	} socket;
} cap_t;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(cap_t) == 8, "cap_t size != 8 bytes");
#endif

static inline bool cap_time_is_child(cap_t parent, cap_t child)
{
	if (child.type == CAPTY_TIME) {
		return parent.time.hartid == child.time.hartid
		       && parent.time.begin <= child.time.begin
		       && child.time.end <= parent.time.end;
	}
	return false;
}

static inline bool cap_memory_is_child(cap_t parent, cap_t child)
{
	if (child.type == CAPTY_MEMORY) {
		return child.memory.offset == parent.memory.offset
		       && parent.memory.begin <= child.memory.begin
		       && child.memory.end <= parent.memory.end;
	}
	if (child.type == CAPTY_PMP) {
		uint64_t parent_begin = (parent.memory.offset << 27)
					+ (parent.memory.begin << 12);
		uint64_t parent_end
		    = (parent.memory.offset << 27) + (parent.memory.end << 12);
		uint64_t child_addr = child.pmp.addr;
		uint64_t child_begin = pmp_napot_begin(child_addr);
		uint64_t child_end = pmp_napot_end(child_addr);
		return parent_begin <= child_begin && child_end <= parent_end;
	}
	return false;
}

static inline bool cap_channel_is_child(cap_t parent, cap_t child)
{
	if (child.type == CAPTY_CHANNEL)
		return child.type == CAPTY_CHANNEL
		       && parent.channel.begin <= child.channel.begin
		       && child.channel.end <= parent.channel.end;
	if (child.type == CAPTY_SOCKET)
		return parent.channel.begin <= child.channel.begin
		       && child.channel.end < parent.channel.end;
	return false;
}

static inline bool cap_socket_is_child(cap_t parent, cap_t child)
{
	if (child.type == CAPTY_SOCKET)
		return parent.socket.tag == 0
		       && parent.socket.channel == child.socket.channel;
	return false;
}

static inline bool cap_monitor_is_child(cap_t parent, cap_t child)
{
	if (child.type == CAPTY_MONITOR)
		return parent.monitor.begin <= child.monitor.begin
		       && child.monitor.end <= parent.monitor.end;
	return false;
}

static inline bool cap_is_child(cap_t parent, cap_t child)
{
	switch (parent.type) {
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
	if (child.type == CAPTY_TIME)
		return parent.time.hartid == child.time.hartid
		       && parent.time.free == child.time.begin
		       && child.time.end <= parent.time.end
		       && child.time.begin == child.time.free
		       && child.time.begin <= parent.time.end;
	return false;
}

static inline bool cap_memory_can_derive(cap_t parent, cap_t child)
{
	if (child.type == CAPTY_MEMORY) {
		return parent.memory.offset == child.memory.offset
		       && parent.memory.free == child.memory.begin
		       && child.memory.end <= parent.memory.end
		       && child.memory.begin == child.memory.free
		       && child.memory.begin < child.memory.end
		       && (child.memory.rwx & parent.memory.rwx)
			      == child.memory.rwx
		       && child.memory.lock == 0 && parent.memory.lock == 0;
	}
	if (child.type == CAPTY_PMP) {
		uint64_t parent_free
		    = (parent.memory.offset << 27) + (parent.memory.free << 12);
		uint64_t parent_end
		    = (parent.memory.offset << 27) + (parent.memory.free << 12);
		uint64_t child_begin = pmp_napot_begin(child.pmp.addr);
		uint64_t child_end = pmp_napot_end(child.pmp.addr);
		return parent_free <= child_begin && child_end <= parent_end
		       && (child.pmp.rwx & parent.memory.rwx) == child.pmp.rwx;
	}
	return false;
}

static inline bool cap_monitor_can_derive(cap_t parent, cap_t child)
{
	if (child.type == CAPTY_MONITOR) {
		return parent.monitor.free == child.monitor.begin
		       && child.monitor.end <= parent.monitor.end
		       && child.monitor.begin == child.monitor.free
		       && child.monitor.begin < child.monitor.end;
	}
	return false;
}

static inline bool cap_channel_can_derive(cap_t parent, cap_t child)
{
	if (child.type == CAPTY_CHANNEL) {
		return parent.channel.free == child.channel.begin
		       && child.channel.end <= parent.channel.end
		       && child.channel.begin == child.channel.free
		       && child.channel.begin < child.channel.end;
	}
	if (child.type == CAPTY_SOCKET) {
		return parent.channel.free == child.socket.channel
		       && child.socket.channel < parent.channel.end
		       && child.socket.tag == 0;
	}
	return false;
}

static inline bool cap_socket_can_derive(cap_t parent, cap_t child)
{
	if (child.type == CAPTY_SOCKET) {
		return parent.socket.channel == child.socket.channel
		       && parent.socket.tag == 0 && 0 < child.socket.tag;
	}
	return false;
}

static inline bool cap_can_derive(cap_t parent, cap_t child)
{
	switch (parent.type) {
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

#pragma once

#include "excpt.h"

#include <stdint.h>

#define CAP_RWX 0x7
#define CAP_RW 0x3
#define CAP_RX 0x5
#define CAP_R 0x1

typedef enum {
	CAPTY_NULL,    ///< No capability.
	CAPTY_TIME,    ///< Time Slice capability.
	CAPTY_MEMORY,  ///< Memory Slice capability.
	CAPTY_PMP,     ///< PMP Frame capability.
	CAPTY_MONITOR, ///< Monitor capability.
	CAPTY_CHANNEL, ///< IPC Channel capability.
	CAPTY_SOCKET,  ///< IPC Socket capability.
	CAPTY_COUNT,   ///< Number of capabilitiy types.
} cap_type_t;

typedef struct {
	uint64_t type : 4;
	uint64_t unused : 4;
	uint64_t hartid : 8;
	uint64_t begin : 16;
	uint64_t free : 16;
	uint64_t end : 16;
} cap_time_t;

typedef struct {
	uint64_t type : 4;
	uint64_t rwx : 3;
	uint64_t lock : 1;
	uint64_t offset : 8;
	uint64_t begin : 16;
	uint64_t free : 16;
	uint64_t end : 16;
} cap_memory_t;

typedef struct {
	uint64_t type : 4;
	uint64_t rwx : 3;
	uint64_t used : 1;
	uint64_t index : 4;
	uint64_t addr : 48;
} cap_pmp_t;

typedef struct {
	uint64_t type : 4;
	uint64_t unused : 12;
	uint64_t begin : 16;
	uint64_t free : 16;
	uint64_t end : 16;
} cap_monitor_t;

typedef struct {
	uint64_t type : 4;
	uint64_t unused : 12;
	uint64_t begin : 16;
	uint64_t free : 16;
	uint64_t end : 16;
} cap_channel_t;

typedef struct {
	uint64_t type : 4;
	uint64_t unused : 28;
	uint64_t channel : 16;
	uint64_t tag : 16;
} cap_socket_t;

/// Capability description
typedef union {
	uint64_t type : 4;
	uint64_t raw;
	cap_time_t time;
	cap_memory_t memory;
	cap_pmp_t pmp;
	cap_monitor_t monitor;
	cap_channel_t channel;
	cap_socket_t socket;
} cap_t;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(cap_t) == 8, "cap_t size != 8 bytes");
#endif

#define CAP_PMP(_addr, _rwx)               \
	(cap_t)                            \
	{                                  \
		.pmp = {.type = CAPTY_PMP, \
			.rwx = _rwx,       \
			.used = 0,         \
			.index = 0,        \
			.addr = _addr }    \
	}
#define CAP_MEMORY(_begin, _end, _offset, _rwx)  \
	(cap_t)                                  \
	{                                        \
		.memory = {.type = CAPTY_MEMORY, \
			   .rwx = _rwx,          \
			   .lock = 0,            \
			   .offset = _offset,    \
			   .begin = _begin,      \
			   .free = _begin,       \
			   .end = _end }         \
	}
#define CAP_TIME(_hartid, _begin, _end)      \
	(cap_t)                              \
	{                                    \
		.time = {.type = CAPTY_TIME, \
			 .hartid = _hartid,  \
			 .begin = _begin,    \
			 .free = _begin,     \
			 .end = _end }       \
	}
#define CAP_MONITOR(_begin, _end)                  \
	(cap_t)                                    \
	{                                          \
		.monitor = {.type = CAPTY_MONITOR, \
			    .begin = _begin,       \
			    .free = _begin,        \
			    .end = _end }          \
	}
#define CAP_CHANNEL(_begin, _end)                  \
	(cap_t)                                    \
	{                                          \
		.channel = {.type = CAPTY_CHANNEL, \
			    .begin = _begin,       \
			    .free = _begin,        \
			    .end = _end }          \
	}

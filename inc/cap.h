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

#include <stdbool.h>
#include <stdint.h>

#define CAP_RWX 0x7
#define CAP_RW 0x3
#define CAP_RX 0x5
#define CAP_R 0x1

#define CAP_NULL         \
	{                \
		.raw = 0 \
	}

#define CAP_TIME(_hartid, _begin, _end)    \
	{                                  \
		.time                      \
		    = {.type = CAPTY_TIME, \
		       .unused = 0,        \
		       .hartid = _hartid,  \
		       .begin = _begin,    \
		       .free = _begin,     \
		       .end = _end }       \
	}

#define CAP_MEMORY(_begin, _end, _offset, _rwx) \
	{                                       \
		.memory                         \
		    = {.type = CAPTY_MEMORY,    \
		       .lock = 0,               \
		       .rwx = _rwx,             \
		       .offset = _offset,       \
		       .begin = _begin,         \
		       .free = _begin,          \
		       .end = _end }            \
	}

#define CAP_PMP(_addr, _cfg)                                                   \
	{                                                                      \
		.pmp = {.type = CAPTY_PMP, .addr = _addr, .cfg = 0x18 | _cfg } \
	}

#define CAP_MONITOR(_begin, _end)             \
	{                                     \
		.monitor                      \
		    = {.type = CAPTY_MONITOR, \
		       .unused = 0,           \
		       .begin = _begin,       \
		       .free = _begin,        \
		       .end = _end }          \
	}

#define CAP_CHANNEL(_begin, _end)             \
	{                                     \
		.channel                      \
		    = {.type = CAPTY_CHANNEL, \
		       .unused = 0,           \
		       .begin = _begin,       \
		       .free = _begin,        \
		       .end = _end }          \
	}

#define CAP_SOCKET(_channel, _tag)           \
	{                                    \
		.socket                      \
		    = {.type = CAPTY_SOCKET, \
		       .unused = 0,          \
		       .channel = _channel,  \
		       .tag = _tag }         \
	}

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
		uint64_t lock : 1;
		uint64_t rwx : 3;
		uint64_t offset : 8;
		uint64_t begin : 16;
		uint64_t free : 16;
		uint64_t end : 16;
	} memory;

	struct {
		uint64_t type : 4;
		uint64_t addr : 52;
		uint64_t cfg : 8;
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

// PMP utils
uint64_t pmp_napot_addr(uint64_t begin, uint64_t end);
uint64_t pmp_napot_begin(uint64_t addr);
uint64_t pmp_napot_end(uint64_t addr);

bool cap_can_derive(cap_t parent, cap_t child);
bool cap_is_parent(cap_t parent, cap_t child);

// Derivation check
bool cap_time_derive(cap_t parent, cap_t child);
bool cap_memory_derive(cap_t parent, cap_t child);
bool cap_monitor_derive(cap_t parent, cap_t child);
bool cap_channel_derive(cap_t parent, cap_t child);
bool cap_socket_derive(cap_t parent, cap_t child);

// Check for parent (revocation)
bool cap_time_parent(cap_t parent, cap_t child);
bool cap_memory_parent(cap_t parent, cap_t child);
bool cap_monitor_parent(cap_t parent, cap_t child);
bool cap_channel_parent(cap_t parent, cap_t child);
bool cap_socket_parent(cap_t parent, cap_t child);

#endif /* __CAP_H__ */

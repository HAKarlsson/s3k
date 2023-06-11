#pragma once

#include "consts.h"

#include <stdint.h>

#define CAP_RWX 0x7
#define CAP_RW 0x3
#define CAP_RX 0x5
#define CAP_R 0x1

typedef struct {
	uint64_t type : 4;
	uint64_t unused : 4;
	uint64_t hartid : 8;
	uint64_t base : 16;
	uint64_t allocated : 16;
	uint64_t length : 16;
} cap_time_t;

typedef struct {
	uint64_t type : 4;
	uint64_t rwx : 3;
	uint64_t lock : 1;
	uint64_t base : 24;
	uint64_t allocated : 16;
	uint64_t length : 16;
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
	uint64_t base : 16;
	uint64_t allocated : 16;
	uint64_t length : 16;
} cap_monitor_t;

typedef struct {
	uint64_t type : 4;
	uint64_t unused : 12;
	uint64_t base : 16;
	uint64_t allocated : 16;
	uint64_t length : 16;
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

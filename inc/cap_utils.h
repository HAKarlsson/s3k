#pragma once

#include "cap_types.h"

#include <stdbool.h>

bool cap_can_revoke(cap_t, cap_t);
bool cap_can_derive(cap_t, cap_t);

#define CAP_NULL         \
	(cap_t)          \
	{                \
		.raw = 0 \
	}

#define CAP_SENTINEL         \
	(cap_t)          \
	{                \
		.raw = (uint64_t)-1 \
	}

#define CAP_PMP(_addr, _rwx)               \
	(cap_t)                            \
	{                                  \
		.pmp = {.type = CAPTY_PMP, \
			.rwx = _rwx,       \
			.used = 0,         \
			.index = 0,        \
			.addr = _addr }    \
	}

#define CAP_MEMORY(_base, _len, _rwx)            \
	(cap_t)                                  \
	{                                        \
		.memory = {.type = CAPTY_MEMORY, \
			   .rwx = _rwx,          \
			   .lock = 0,            \
			   .base = _base,        \
			   .alloc = 0,           \
			   .len = _len }         \
	}

#define CAP_TIME(_hartid, _base, _len)       \
	(cap_t)                              \
	{                                    \
		.time = {.type = CAPTY_TIME, \
			 .hartid = _hartid,  \
			 .base = _base,      \
			 .alloc = 0,         \
			 .len = _len }       \
	}

#define CAP_MONITOR(_base, _len)                   \
	(cap_t)                                    \
	{                                          \
		.monitor = {.type = CAPTY_MONITOR, \
			    .base = _base,         \
			    .alloc = 0,            \
			    .len = _len }          \
	}

#define CAP_CHANNEL(_base, _len)                   \
	(cap_t)                                    \
	{                                          \
		.channel = {.type = CAPTY_CHANNEL, \
			    .base = _base,         \
			    .alloc = 0,            \
			    .len = _len }          \
	}

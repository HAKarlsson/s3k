#pragma once

#include "cap_types.h"
#include "kassert.h"

#include <stdbool.h>
#include <stddef.h>

#define CTABLE_MAX_DEPTH 256

typedef int32_t cptr_t;

typedef struct {
	cptr_t prev, next;
	cap_t cap;
} cte_t;

extern cte_t ctable[NUM_OF_PROCESSES * NUM_OF_CAPABILITIES];

static inline bool cptr_is_valid(cptr_t cptr)
{
	return cptr >= 0;
}

static inline uint64_t cptr_get_pid(cptr_t cptr)
{
	return cptr / NUM_OF_CAPABILITIES;
}

static inline cptr_t cptr_mk(uint64_t pid, uint64_t idx)
{
	kassert(pid < NUM_OF_PROCESSES);
	if (idx >= NUM_OF_CAPABILITIES)
		return -1;
	cptr_t cptr = pid * NUM_OF_CAPABILITIES + idx;
	kassert(cptr_get_pid(cptr) == pid);
	return cptr;
}

static inline int ctable_is_null(cptr_t cptr)
{
	kassert(cptr_is_valid(cptr));
	return ctable[cptr].cap.raw == 0;
}

static inline cap_t ctable_get_cap(cptr_t cptr)
{
	kassert(cptr_is_valid(cptr));
	return ctable[cptr].cap;
}

static inline cptr_t ctable_get_next(cptr_t cptr)
{
	kassert(cptr_is_valid(cptr));
	return ctable[cptr].next;
}

static inline cptr_t ctable_get_prev(cptr_t cptr)
{
	kassert(cptr_is_valid(cptr));
	return ctable[cptr].prev;
}

void ctable_update(cptr_t src, cap_t cap);
void ctable_move(cptr_t src, cptr_t dst, cap_t cap);
void ctable_delete(cptr_t cptr);
void ctable_insert(cptr_t orig, cptr_t dst, cap_t new_cap);

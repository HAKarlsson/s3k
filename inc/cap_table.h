#pragma once

#include "cap_types.h"
#include "excpt.h"

#include <stdbool.h>
#include <stddef.h>

#define CTABLE_MAX_DEPTH 256

typedef int16_t cptr_t;

typedef struct {
	uint32_t depth;
	cptr_t prev, next;
	cap_t cap;
} cte_t;

static inline cptr_t cptr_mk(uint64_t pid, uint64_t idx)
{
	if (idx >= NUM_OF_CAPABILITIES)
		return -1;
	return pid * NUM_OF_CAPABILITIES + idx;
}

static inline bool cptr_is_valid(cptr_t cptr)
{
	return cptr >= 0;
}

static inline uint64_t cptr_get_pid(cptr_t cptr)
{
	return cptr / NUM_OF_CAPABILITIES;
}

bool ctable_is_null(cptr_t cptr);

cap_t ctable_get_cap(cptr_t cptr);
uint32_t ctable_get_depth(cptr_t cptr);
cptr_t ctable_get_next(cptr_t cptr);
cptr_t ctable_get_prev(cptr_t cptr);

void ctable_update(cptr_t src, cap_t cap);
void ctable_move(cptr_t src, cptr_t dst, cap_t cap);
void ctable_delete(cptr_t cptr);
void ctable_insert(cptr_t orig, cptr_t dst, cap_t new_cap);

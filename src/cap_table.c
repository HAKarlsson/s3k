#include "cap_table.h"

#include "cap_utils.h"
#include "common.h"
#include "init_caps.h"
#include "kassert.h"

static cte_t ctable[NUM_OF_PROCESSES * NUM_OF_CAPABILITIES];

void ctable_init(void)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(init_caps); ++i) {
		ctable[i].prev = i - 1;
		ctable[i].next = i + 1;
		ctable[i].cap = init_caps[i];
	}
	ctable[0].prev = ARRAY_SIZE(init_caps) - 1;
	ctable[ARRAY_SIZE(init_caps) - 1].next = 0;
}

bool ctable_is_null(cptr_t cptr)
{
	kassert(cptr_is_valid(cptr));
	return ctable[cptr].cap.raw == 0;
}

cap_t ctable_get_cap(cptr_t cptr)
{
	return ctable[cptr].cap;
}

uint32_t ctable_get_depth(cptr_t cptr)
{
	return ctable[cptr].depth;
}

cptr_t ctable_get_next(cptr_t cptr)
{
	return ctable[cptr].next;
}

void ctable_move(cptr_t src, cptr_t dst, cap_t cap)
{
	kassert(!ctable_is_null(src));
	kassert(ctable_is_null(dst));

	cptr_t prev = ctable[src].prev;
	cptr_t next = ctable[src].next;
	uint32_t depth = ctable[src].depth;

	ctable[dst].prev = prev;
	ctable[dst].next = next;
	ctable[dst].cap = cap;
	ctable[dst].depth = depth;

	ctable[prev].next = dst;
	ctable[next].prev = dst;
	ctable[src].cap = CAP_NULL;
}

void ctable_delete(cptr_t cptr)
{
	kassert(!ctable_is_null(cptr));
	cptr_t prev = ctable[cptr].prev;
	cptr_t next = ctable[cptr].next;
	ctable[prev].next = next;
	ctable[next].prev = prev;
	ctable[cptr].cap = CAP_NULL;
}

void ctable_insert(cptr_t src, cptr_t dst, cap_t new_cap)
{
	kassert(!ctable_is_null(src));
	kassert(ctable_is_null(dst));
	cptr_t next = ctable[src].next;
	ctable[src].next = dst;
	ctable[next].prev = dst;
	ctable[dst].depth = ctable[src].depth + 1;
	ctable[dst].prev = src;
	ctable[dst].next = next;
	ctable[dst].cap = new_cap;
}

void ctable_update(cptr_t cptr, cap_t cap)
{
	kassert(!ctable_is_null(cptr));
	kassert(cap.raw != 0);
	ctable[cptr].cap = cap;
}

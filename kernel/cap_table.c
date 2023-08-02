#include "cap_table.h"

#include "cap_utils.h"
#include "kassert.h"
#include "macro.h"

cte_t ctable[NUM_OF_PROCESSES * NUM_OF_CAPABILITIES];

static cap_t init_caps[] = INIT_CAPS;

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

void ctable_move(cptr_t src, cptr_t dst, cap_t cap)
{
	kassert(cptr_is_valid(src));
	kassert(cptr_is_valid(dst));
	kassert(!ctable_is_null(src));
	kassert(ctable_is_null(dst));

	cptr_t prev = ctable[src].prev;
	cptr_t next = ctable[src].next;

	ctable[dst].prev = prev;
	ctable[dst].next = next;
	ctable[dst].cap = cap;

	ctable[prev].next = dst;
	ctable[next].prev = dst;
	ctable[src].cap = CAP_NULL;
}

void ctable_delete(cptr_t cptr)
{
	kassert(cptr_is_valid(cptr));
	kassert(!ctable_is_null(cptr));
	cptr_t prev = ctable[cptr].prev;
	cptr_t next = ctable[cptr].next;
	ctable[prev].next = next;
	ctable[next].prev = prev;
	ctable[cptr].cap = CAP_NULL;
}

void ctable_insert(cptr_t src, cptr_t dst, cap_t new_cap)
{
	kassert(cptr_is_valid(src));
	kassert(cptr_is_valid(dst));
	kassert(!ctable_is_null(src));
	kassert(ctable_is_null(dst));
	cptr_t next = ctable[src].next;
	ctable[src].next = dst;
	ctable[next].prev = dst;
	ctable[dst].prev = src;
	ctable[dst].next = next;
	ctable[dst].cap = new_cap;
}

void ctable_update(cptr_t cptr, cap_t cap)
{
	kassert(cptr_is_valid(cptr));
	kassert(!ctable_is_null(cptr));
	kassert(cap.raw != 0);
	ctable[cptr].cap = cap;
}

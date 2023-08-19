#include "cap/table.h"

#include "kassert.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

struct cte {
	cidx_t prev, next;
	cap_t cap;
};

static struct cte ctable[N_PROC][N_CAP];

static struct cte *ctable_get(cidx_t i)
{
	return &ctable[i.pid][i.idx];
}

static void ctable_set_next(cidx_t i, cidx_t next)
{
	ctable_get(i)->next = next;
}

static void ctable_set_prev(cidx_t i, cidx_t prev)
{
	ctable_get(i)->prev = prev;
}

static void ctable_set_cap(cidx_t i, cap_t cap)
{
	ctable_get(i)->cap = cap;
}

cidx_t cidx(uint64_t pid, uint64_t idx)
{
	kassert(pid < N_PROC && idx < N_CAP);
	return (cidx_t){ (uint16_t)pid, (uint16_t)idx };
}

bool ctable_is_empty(cidx_t i)
{
	return ctable_get(i)->cap.raw == 0;
}

cidx_t ctable_get_next(cidx_t i)
{
	return ctable_get(i)->next;
}

cidx_t ctable_get_prev(cidx_t i)
{
	return ctable_get(i)->prev;
}

cap_t ctable_get_cap(cidx_t i)
{
	return ctable_get(i)->cap;
}

void ctable_init(void)
{
	const cap_t init_caps[] = INIT_CAPS;
	const size_t size = ARRAY_SIZE(init_caps);
	for (unsigned int i = 0; i < size; ++i) {
		ctable[0][i].prev = cidx(0, i - 1);
		ctable[0][i].next = cidx(0, i + 1);
		ctable[0][i].cap = init_caps[i];
	}
	ctable[0][0].prev = cidx(0, size - 1);
	ctable[0][size - 1].next = cidx(0, 0);
}

void ctable_move(cidx_t src, cap_t cap, cidx_t dst)
{
	cidx_t prev = ctable_get_prev(src);
	kassert(!ctable_is_empty(prev));
	ctable_delete(src);
	ctable_insert(dst, cap, prev);
}

void ctable_delete(cidx_t idx)
{
	cidx_t prev = ctable_get_prev(idx);
	cidx_t next = ctable_get_next(idx);
	ctable_set_next(prev, next);
	ctable_set_prev(next, prev);
	ctable_set_cap(idx, (cap_t){ .raw = 0 });
}

bool ctable_conditional_delete(cidx_t idx, cap_t cap, cidx_t prev)
{
	if (ctable_get_prev(idx).pid != prev.pid
	    || ctable_get_prev(idx).idx != prev.idx
	    || ctable_get_cap(idx).raw != cap.raw)
		return false;
	ctable_delete(idx);
	return true;
}

void ctable_insert(cidx_t idx, cap_t cap, cidx_t prev)
{
	cidx_t next = ctable_get_next(prev);
	ctable_set_next(prev, idx);
	ctable_set_prev(next, idx);

	ctable_set_prev(idx, prev);
	ctable_set_next(idx, next);
	ctable_set_cap(idx, cap);
}

void ctable_update(cidx_t idx, cap_t cap)
{
	ctable_set_cap(idx, cap);
}

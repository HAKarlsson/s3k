/* See LICENSE file for copyright and license details. */
#include "cnode.h"

#include "cap.h"
#include "common.h"
#include "consts.h"
#include "excpt.h"
#include "init_caps.h"
#include "kassert.h"
#include "ticket_lock.h"

struct cnode {
	cap_t cap;
	cnode_handle_t prev;
	cnode_handle_t next;
};

cnode_t _cnodes[PROC_COUNT * CAP_COUNT];

cnode_handle_t cnode_get(uint64_t pid, uint64_t idx)
{
	kassert(pid < PROC_COUNT);
	kassert(idx < CAP_COUNT);
	return pid * CAP_COUNT + idx;
}

cnode_handle_t cnode_next(cnode_handle_t node)
{
	return _cnodes[node].next;
}

cnode_handle_t cnode_prev(cnode_handle_t node)
{
	return _cnodes[node].prev;
}

uint64_t cnode_pid(cnode_handle_t node)
{
	return node / CAP_COUNT;
}

void cnode_set_next(cnode_handle_t node, cnode_handle_t next)
{
	_cnodes[node].next = next;
}

void cnode_set_prev(cnode_handle_t node, cnode_handle_t prev)
{
	_cnodes[node].prev = prev;
}

cap_t cnode_cap(const cnode_handle_t node)
{
	return _cnodes[node].cap;
}

void cnode_set_cap(cnode_handle_t node, cap_t cap)
{
	_cnodes[node].cap.raw = cap.raw;
}

bool cnode_is_null(cnode_handle_t node)
{
	return cnode_cap(node).raw == 0;
}

void cnode_init(void)
{
	kassert(ARRAY_SIZE(init_caps) < CAP_COUNT);

	cnode_handle_t first = cnode_get(0, 0);
	cnode_handle_t last = cnode_get(0, 0);

	// Add initial capabilities.
	for (size_t i = 0; i < ARRAY_SIZE(init_caps); ++i) {
		cnode_handle_t node = cnode_get(0, i);
		cnode_set_next(last, cnode_next(node));
		cnode_set_next(first, cnode_next(node));
		cnode_set_prev(node, last);
		cnode_set_next(node, first);
		cnode_set_cap(node, init_caps[i]);
		last = node;
	}
}

bool cnode_update(cnode_handle_t node, cap_t cap)
{
	if (cnode_is_null(node))
		return false;
	cnode_set_cap(node, cap);
	return true;
}

bool cnode_insert(cnode_handle_t node, cap_t cap, cnode_handle_t prev)
{
	if (cnode_is_null(prev) || !cnode_is_null(node))
		return false;
	cnode_handle_t next = cnode_next(prev);
	cnode_set_prev(next, node);
	cnode_set_next(node, next);
	cnode_set_prev(node, prev);
	cnode_set_next(prev, node);
	cnode_set_cap(node, cap);
	return true;
}

bool cnode_move(cnode_handle_t src, cnode_handle_t dst)
{
	if (cnode_is_null(src) || !cnode_is_null(dst))
		return false;
	cnode_handle_t next = cnode_next(src);
	cnode_handle_t prev = cnode_prev(src);
	cnode_set_next(dst, next);
	cnode_set_prev(dst, prev);
	cnode_set_next(prev, dst);
	cnode_set_prev(next, dst);
	cnode_set_cap(dst, cnode_cap(src));
	return true;
}

bool cnode_delete(cnode_handle_t node)
{
	if (cnode_is_null(node))
		return false;
	cnode_handle_t next = cnode_next(node);
	cnode_handle_t prev = cnode_prev(node);
	cnode_set_prev(next, prev);
	cnode_set_prev(prev, next);
	cnode_set_cap(node, CAP_NONE);
	return true;
}

bool cnode_delete_if(cnode_handle_t node, cap_t cap, cnode_handle_t prev)
{
	if (cnode_prev(node) != prev || cnode_cap(node).raw != cap.raw)
		return false;
	return cnode_delete(node);
}

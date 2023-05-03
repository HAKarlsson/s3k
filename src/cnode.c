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

ticket_lock_t _cnode_lock;
cnode_t _cnodes[NPROC * NCAP];

cnode_handle_t cnode_get(uint64_t pid, uint64_t idx)
{
	kassert(pid < NPROC);
	kassert(idx < NCAP);
	return pid * NCAP + idx;
}

cnode_handle_t cnode_next(cnode_handle_t node)
{
	return _cnodes[node].next;
}

cnode_handle_t cnode_prev(cnode_handle_t node)
{
	return _cnodes[node].prev;
}

void cnode_setnext(cnode_handle_t node, cnode_handle_t next)
{
	_cnodes[node].next = next;
}

void cnode_setprev(cnode_handle_t node, cnode_handle_t prev)
{
	_cnodes[node].prev = prev;
}

cap_t cnode_cap(const cnode_handle_t node)
{
	return _cnodes[node].cap;
}

void cnode_setcap(cnode_handle_t node, cap_t cap)
{
	_cnodes[node].cap.raw = cap.raw;
}

bool cnode_is_null(cnode_handle_t node)
{
	return cnode_cap(node).raw == 0;
}

void cnode_init(void)
{
	kassert(ARRAY_SIZE(init_caps) < NCAP);

	cnode_handle_t first = cnode_get(0, 0);
	cnode_handle_t last = cnode_get(0, 0);

	// Add initial capabilities.
	for (size_t i = 0; i < ARRAY_SIZE(init_caps); ++i) {
		cnode_handle_t node = cnode_get(0, i);
		cnode_setnext(last, cnode_next(node));
		cnode_setnext(first, cnode_next(node));
		cnode_setprev(node, last);
		cnode_setnext(node, first);
		cnode_setcap(node, init_caps[i]);
		last = node;
	}
}

excpt_t cnode_update(cnode_handle_t node, cap_t cap)
{
	excpt_t status;
	tl_acq(&_cnode_lock);
	if (cnode_is_null(node)) {
		status = EXCPT_EMPTY;
	} else {
		status = EXCPT_NONE;
		cnode_setcap(node, cap);
	}
	tl_rel(&_cnode_lock);
	return status;
}

excpt_t cnode_insert(cnode_handle_t node, cap_t cap, cnode_handle_t prev)
{
	excpt_t status;
	tl_acq(&_cnode_lock);
	if (!cnode_is_null(node)) {
		status = EXCPT_COLLISION;
	} else if (cnode_is_null(prev)) {
		status = EXCPT_EMPTY;
	} else {
		status = EXCPT_NONE;
		cnode_handle_t next = cnode_next(prev);

		// Connect with next
		cnode_setprev(next, node);
		cnode_setnext(node, next);

		// connect with prev
		cnode_setprev(node, prev);
		cnode_setnext(prev, node);

		cnode_setcap(node, cap);
	}
	tl_rel(&_cnode_lock);
	return status;
}

excpt_t cnode_move(cnode_handle_t src, cnode_handle_t dst)
{
	excpt_t status;
	tl_acq(&_cnode_lock);
	if (!cnode_is_null(dst)) {
		status = EXCPT_COLLISION;
	} else if (cnode_is_null(src)) {
		status = EXCPT_EMPTY;
	} else {
		status = EXCPT_NONE;
		// Fix the links
		cnode_handle_t next = cnode_next(src);
		cnode_handle_t prev = cnode_prev(src);
		cnode_setnext(dst, next);
		cnode_setprev(dst, prev);
		cnode_setnext(prev, dst);
		cnode_setprev(next, dst);
		// Set the capability
		cnode_setcap(dst, cnode_cap(src));
	}
	tl_rel(&_cnode_lock);
	return status;
}

excpt_t cnode_delete(cnode_handle_t node)
{
	excpt_t status;
	tl_acq(&_cnode_lock);
	if (cnode_is_null(node)) {
		status = EXCPT_EMPTY;
	} else {
		status = EXCPT_NONE;
		cnode_handle_t next = cnode_next(node);
		cnode_handle_t prev = cnode_prev(node);
		cnode_setprev(next, prev);
		cnode_setprev(prev, next);
		cnode_setcap(node, CAP_NONE);
	}
	tl_rel(&_cnode_lock);
	return status;
}

excpt_t cnode_delete_if(cnode_handle_t node, cap_t cap, cnode_handle_t prev)
{
	excpt_t status;
	tl_acq(&_cnode_lock);
	if (cnode_is_null(node)) {
		status = EXCPT_EMPTY;
	} else if (cnode_prev(node) != prev || cnode_cap(node).raw != cap.raw) {
		status = EXCPT_UNSPECIFIED;
	} else {
		status = EXCPT_NONE;
		cnode_handle_t next = cnode_next(node);
		cnode_setprev(next, prev);
		cnode_setnext(prev, next);
		cnode_setcap(node, CAP_NONE);
	}
	tl_rel(&_cnode_lock);
	return status;
}

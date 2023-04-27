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
	uint32_t prev, next;
	cap_t cap;
};

ticket_lock_t _cnode_lock;
static struct cnode _cnodes[NPROC * NCAP];

void cnode_init(void)
{
	kassert(ARRAY_SIZE(init_caps) < NCAP);

	int first = 0, last = 0;

	// Add initial capabilities.
	for (size_t i = 0; i < ARRAY_SIZE(init_caps); ++i) {
		_cnodes[last].next = i;
		_cnodes[first].prev = i;
		_cnodes[i].prev = last;
		_cnodes[i].next = first;
		_cnodes[i].cap = init_caps[i];
		last = i;
	}
}

cap_t cnode_cap(uint64_t idx)
{
	return _cnodes[idx].cap;
}

uint64_t cnode_next(uint64_t idx)
{
	return _cnodes[idx].next;
}

uint64_t cnode_prev(uint64_t idx)
{
	return _cnodes[idx].prev;
}

bool cnode_contains(uint64_t idx)
{
	return _cnodes[idx].cap.raw != 0;
}

excpt_t cnode_update(uint64_t idx, cap_t cap)
{
	excpt_t status;
	tl_acq(&_cnode_lock);
	if (!cnode_contains(idx)) {
		status = EXCPT_EMPTY;
	} else {
		status = EXCPT_NONE;
		_cnodes[idx].cap = cap;
	}
	tl_rel(&_cnode_lock);
	return status;
}

excpt_t cnode_insert(uint64_t idx_new, cap_t cap_new, uint64_t idx_prev)
{
	excpt_t status;
	tl_acq(&_cnode_lock);
	if (cnode_contains(idx_new)) {
		status = EXCPT_COLLISION;
	} else if (!cnode_contains(idx_prev)) {
		status = EXCPT_EMPTY;
	} else {
		status = EXCPT_NONE;
		_cnodes[_cnodes[idx_prev].next].prev = idx_new;
		_cnodes[idx_new].next = _cnodes[idx_prev].next;
		_cnodes[idx_new].prev = idx_prev;
		_cnodes[idx_new].cap = cap_new;
		_cnodes[idx_prev].next = idx_new;
	}
	tl_rel(&_cnode_lock);
	return status;
}

excpt_t cnode_move(uint64_t idx_src, uint64_t idx_dst)
{
	excpt_t status;
	tl_acq(&_cnode_lock);
	if (cnode_contains(idx_dst)) {
		status = EXCPT_COLLISION;
	} else if (!cnode_contains(idx_src)) {
		status = EXCPT_EMPTY;
	} else {
		status = EXCPT_NONE;
		_cnodes[idx_dst].cap = _cnodes[idx_src].cap;
		_cnodes[idx_dst].next = _cnodes[idx_src].next;
		_cnodes[idx_dst].prev = _cnodes[idx_src].prev;
		_cnodes[_cnodes[idx_dst].prev].next = idx_dst;
		_cnodes[_cnodes[idx_dst].next].prev = idx_dst;
		_cnodes[idx_src].cap.raw = 0;
	}
	tl_rel(&_cnode_lock);
	return status;
}

excpt_t cnode_delete(uint64_t idx)
{
	excpt_t status;
	tl_acq(&_cnode_lock);
	if (!cnode_contains(idx)) {
		status = EXCPT_EMPTY;
	} else {
		status = EXCPT_NONE;
		_cnodes[_cnodes[idx].next].prev = _cnodes[idx].prev;
		_cnodes[_cnodes[idx].prev].next = _cnodes[idx].next;
		_cnodes[idx].cap.raw = 0;
	}
	tl_rel(&_cnode_lock);
	return status;
}

excpt_t cnode_delete_if(uint64_t idx, uint64_t idx_prev)
{
	excpt_t status;
	tl_acq(&_cnode_lock);
	if (!cnode_contains(idx)) {
		status = EXCPT_EMPTY;
	} else if (cnode_prev(idx) != idx_prev) {
		status = EXCPT_UNSPECIFIED;
	} else {
		status = EXCPT_NONE;
		_cnodes[_cnodes[idx].next].prev = _cnodes[idx].prev;
		_cnodes[_cnodes[idx].prev].next = _cnodes[idx].next;
		_cnodes[idx].cap.raw = 0;
	}
	tl_rel(&_cnode_lock);
	return status;
}

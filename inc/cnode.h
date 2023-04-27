/**
 * @file cnode.h
 * @brief Functions for handling capabilities tree.
 * @copyright MIT License
 * @author Henrik Karlsson (henrik10@kth.se)
 */
#ifndef __CNODE_H__
#define __CNODE_H__

#include "cap.h"
#include "excpt.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// @defgroup cnode Capability Node
///
/// Kernel internal module for handling capability tree.
///
/// The capability tree is not an actual tree, it is a linked list backed by an
/// array. The tree properties exists implicitly by the relations of
/// capabilities.
///

/**
 * @brief Initialize the cnode structure.
 */
void cnode_init(void);

static inline uint64_t cnode_idx(uint64_t pid, uint64_t idx)
{
	return pid * NCAP + idx;
}

cap_t cnode_cap(uint64_t idx);
uint64_t cnode_next(uint64_t idx);
uint64_t cnode_prev(uint64_t idx);
bool cnode_contains(uint64_t idx);
excpt_t cnode_update(uint64_t idx, cap_t cap);
excpt_t cnode_insert(uint64_t idx_new, cap_t cap_new, uint64_t idx_prev);
excpt_t cnode_move(uint64_t idx_src, uint64_t idx_dst);
excpt_t cnode_delete_if(uint64_t idx, uint64_t pred_idx);
excpt_t cnode_delete(uint64_t idx);
#endif /* __CNODE_H__ */

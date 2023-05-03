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
#include "proc.h"

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

typedef struct cnode cnode_t;
typedef uint32_t cnode_handle_t;

/**
 * @brief Initialize the cnode structure.
 */
void cnode_init(void);

cnode_handle_t cnode_get(uint64_t pid, uint64_t idx);
cap_t cnode_cap(const cnode_handle_t node);
cnode_handle_t cnode_next(const cnode_handle_t node);
cnode_handle_t cnode_prev(const cnode_handle_t node);
bool cnode_is_null(cnode_handle_t node);
excpt_t cnode_update(cnode_handle_t node, cap_t cap);
excpt_t cnode_insert(cnode_handle_t node, cap_t cap, cnode_handle_t prev);
excpt_t cnode_move(cnode_handle_t src, cnode_handle_t dst);
excpt_t cnode_delete(cnode_handle_t node);
excpt_t cnode_delete_if(cnode_handle_t node, cap_t cap, cnode_handle_t pred);
#endif /* __CNODE_H__ */

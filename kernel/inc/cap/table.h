#ifndef CAP_CTABLE_H
#define CAP_CTABLE_H

#include "cap/types.h"
#include "error.h"
#include "kassert.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct cidx {
	uint16_t pid, idx;
} cidx_t;

void ctable_init();
cidx_t cidx(uint64_t pid, uint64_t idx);
bool ctable_is_empty(cidx_t i);
cidx_t ctable_get_next(cidx_t i);
cidx_t ctable_get_prev(cidx_t i);
cap_t ctable_get_cap(cidx_t i);
void ctable_move(cidx_t src, cap_t cap, cidx_t dst);
void ctable_delete(cidx_t idx);
bool ctable_conditional_delete(cidx_t idx, cap_t cap, cidx_t prev);
void ctable_insert(cidx_t idx, cap_t cap, cidx_t prev);
void ctable_update(cidx_t idx, cap_t cap);

#endif /* CAP_CTABLE_H */

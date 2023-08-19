#pragma once

#include "cap/table.h"
#include "error.h"

#include <stdint.h>

bool check_preemption(void);
bool check_invalid_cslot(uint64_t i);
bool check_src_empty(cidx_t src);
bool check_dst_occupied(cidx_t src);
bool check_invalid_derivation(cap_t src_cap, cap_t new_cap);
bool check_invalid_capty(cap_t cap, capty_t ty);
bool check_invalid_pmpidx(uint64_t pmpidx);
bool check_invalid_register(uint64_t reg);
bool check_pmp_used(cap_t cap);
bool check_pmp_unused(cap_t cap);
bool check_pmp_occupied(uint64_t pid, uint64_t pmpidx);
bool check_mon_invalid_pid(cap_t cap, uint64_t pid);

void delete_cap(cidx_t src, cap_t cap);
void move_cap(cidx_t src, cap_t cap, cidx_t dst);
void try_revoke_cap(cidx_t parent, cap_t parent_cap, cidx_t child_dst,
		    cap_t child_cap);
void restore_cap(cidx_t idx, cap_t cap);
void derive_cap(cidx_t src, cap_t src_cap, cidx_t dst, cap_t new_cap);
void pmp_load(cidx_t idx, cap_t cap, uint64_t pmpidx);
void pmp_unload(cidx_t idx, cap_t cap);

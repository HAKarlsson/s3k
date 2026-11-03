#pragma once
#include "s3k/types.h"

enum {
	S3K_SYSCALL_PID_GET,
	S3K_SYSCALL_VREG_GET,
	S3K_SYSCALL_VREG_SET,
	S3K_SYSCALL_SYNC,
	S3K_SYSCALL_SLEEP_UNTIL,
	S3K_SYSCALL_CAP_INTROSPECT,
	S3K_SYSCALL_CAP_DERIVE,
	S3K_SYSCALL_CAP_REVOKE,
	S3K_SYSCALL_CAP_DELETE,
	S3K_SYSCALL_MEM_PMP_GET,
	S3K_SYSCALL_MEM_PMP_SET,
	S3K_SYSCALL_TSL_SET,
	S3K_SYSCALL_MON_SUSPEND,
	S3K_SYSCALL_MON_RESUME,
	S3K_SYSCALL_MON_YIELD,
	S3K_SYSCALL_MON_REG_GET,
	S3K_SYSCALL_MON_REG_SET,
	S3K_SYSCALL_MON_VREG_GET,
	S3K_SYSCALL_MON_VREG_SET,
	S3K_SYSCALL_MON_STATE_GET,
	S3K_SYSCALL_MON_CAP_INTROSPECT,
	S3K_SYSCALL_MON_GRANT,
	S3K_SYSCALL_MON_CAP_DERIVE,
	S3K_SYSCALL_MON_MEM_PMP_GET,
	S3K_SYSCALL_MON_MEM_PMP_SET,
	S3K_SYSCALL_MON_TSL_SET,
	S3K_SYSCALL_IPC_SEND,
	S3K_SYSCALL_IPC_RECV,
	S3K_SYSCALL_IPC_CALL,
	S3K_SYSCALL_IPC_REPLY,
	S3K_SYSCALL_IPC_REPLYRECV,
	S3K_SYSCALL_IPC_ASEND,
	S3K_SYSCALL_IPC_ARECV,
};

static inline s3k_pid_t s3k_pid_get(void)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_PID_GET;
	__asm__ volatile("ecall" : "+r"(a0));
	return a0;
}

static inline s3k_word_t s3k_vreg_get(s3k_vreg_t reg)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_VREG_GET;
	register s3k_word_t a1 __asm__("a1") = reg;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1));
	return a0;
}

static inline void s3k_vreg_set(s3k_vreg_t reg, s3k_word_t val)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_VREG_SET;
	register s3k_word_t a1 __asm__("a1") = reg;
	register s3k_word_t a2 __asm__("a2") = val;
	__asm__ volatile("ecall" ::"r"(a0), "r"(a1), "r"(a2));
}

static inline void s3k_sync(void)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_SYNC;
	__asm__ volatile("ecall" : "+r"(a0) : : "memory");
}

static inline void s3k_sleep_until(s3k_time_t time)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_SLEEP_UNTIL;
	register s3k_word_t a1 __asm__("a1") = time;
	__asm__ volatile("ecall" ::"r"(a0), "r"(a1));
}

static inline int s3k_cap_introspect(s3k_capty_t type, s3k_index_t i, s3k_fuel_t offset, void *cap)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_CAP_INTROSPECT;
	register s3k_word_t a1 __asm__("a1") = type;
	register s3k_word_t a2 __asm__("a2") = i;
	register s3k_word_t a3 __asm__("a3") = offset;
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1), "+r"(a2) : "r"(a3));
	if (a0 == S3K_SUCCESS) {
		s3k_word_t *ptr = (s3k_word_t *)cap;
		ptr[0] = a1;
		if (type != S3K_CAPTY_MON) {
			ptr[1] = a2;
		}
	}
	return a0;
}

static inline int s3k_mem_introspect(s3k_index_t i, s3k_fuel_t offset, s3k_cap_mem_t *cap)
{
	return s3k_cap_introspect(S3K_CAPTY_MEM, i, offset, cap);
}

static inline int s3k_mem_get(s3k_index_t i, s3k_cap_mem_t *cap)
{
	return s3k_mem_introspect(i, 0, cap);
}

static inline int s3k_tsl_introspect(s3k_index_t i, s3k_fuel_t offset, s3k_cap_tsl_t *cap)
{
	return s3k_cap_introspect(S3K_CAPTY_TSL, i, offset, cap);
}

static inline int s3k_tsl_get(s3k_index_t i, s3k_cap_tsl_t *cap)
{
	return s3k_tsl_introspect(i, 0, cap);
}

static inline int s3k_mon_introspect(s3k_index_t i, s3k_fuel_t offset, s3k_cap_mon_t *cap)
{
	return s3k_cap_introspect(S3K_CAPTY_MON, i, offset, cap);
}

static inline int s3k_mon_get(s3k_index_t i, s3k_cap_mon_t *cap)
{
	return s3k_mon_introspect(i, 0, cap);
}

static inline int s3k_ipc_introspect(s3k_index_t i, s3k_fuel_t offset, s3k_cap_ipc_t *cap)
{
	return s3k_cap_introspect(S3K_CAPTY_IPC, i, offset, cap);
}

static inline int s3k_ipc_get(s3k_index_t i, s3k_cap_ipc_t *cap)
{
	return s3k_ipc_introspect(i, 0, cap);
}

static inline int s3k_cap_derive(s3k_capty_t type, s3k_index_t i, s3k_fuel_t csize,
			       s3k_word_t option0, s3k_word_t option1, s3k_word_t option2)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_CAP_DERIVE;
	register s3k_word_t a1 __asm__("a1") = type;
	register s3k_word_t a2 __asm__("a2") = i;
	register s3k_word_t a3 __asm__("a3") = csize;
	register s3k_word_t a4 __asm__("a4") = option0;
	register s3k_word_t a5 __asm__("a5") = option1;
	register s3k_word_t a6 __asm__("a6") = option2;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6));
	return a0;
}

static inline int s3k_mem_derive(s3k_index_t i, s3k_fuel_t csize, s3k_mem_perm_t perm, s3k_mem_addr_t base,
				 s3k_mem_addr_t size)
{
	return s3k_cap_derive(S3K_CAPTY_MEM, i, csize, perm, base, size);
}

static inline int s3k_tsl_derive(s3k_index_t i, s3k_fuel_t csize, bool enabled, s3k_time_slot_t size)
{
	return s3k_cap_derive(S3K_CAPTY_TSL, i, csize, enabled, size, 0);
}

static inline int s3k_mon_derive(s3k_index_t i, s3k_fuel_t csize)
{
	return s3k_cap_derive(S3K_CAPTY_MON, i, csize, 0, 0, 0);
}

static inline int s3k_ipc_derive(s3k_index_t i, s3k_fuel_t csize, s3k_ipc_mode_t mode, s3k_ipc_flag_t flag)
{
	return s3k_cap_derive(S3K_CAPTY_IPC, i, csize, mode, flag, 0);
}

static inline int s3k_cap_revoke(s3k_capty_t type, s3k_index_t i)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_CAP_REVOKE;
	register s3k_word_t a1 __asm__("a1") = type;
	register s3k_word_t a2 __asm__("a2") = i;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2));
	return a0;
}

static inline int s3k_cap_delete(s3k_capty_t type, s3k_index_t i)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_CAP_DELETE;
	register s3k_word_t a1 __asm__("a1") = type;
	register s3k_word_t a2 __asm__("a2") = i;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2));
	return a0;
}

static inline int s3k_mem_revoke(s3k_index_t i)
{
	return s3k_cap_revoke(S3K_CAPTY_MEM, i);
}

static inline int s3k_tsl_revoke(s3k_index_t i)
{
	return s3k_cap_revoke(S3K_CAPTY_TSL, i);
}

static inline int s3k_mon_revoke(s3k_index_t i)
{
	return s3k_cap_revoke(S3K_CAPTY_MON, i);
}

static inline int s3k_ipc_revoke(s3k_index_t i)
{
	return s3k_cap_revoke(S3K_CAPTY_IPC, i);
}

static inline int s3k_mem_delete(s3k_index_t i)
{
	return s3k_cap_delete(S3K_CAPTY_MEM, i);
}

static inline int s3k_tsl_delete(s3k_index_t i)
{
	return s3k_cap_delete(S3K_CAPTY_TSL, i);
}

static inline int s3k_mon_delete(s3k_index_t i)
{
	return s3k_cap_delete(S3K_CAPTY_MON, i);
}

static inline int s3k_ipc_delete(s3k_index_t i)
{
	return s3k_cap_delete(S3K_CAPTY_IPC, i);
}

static inline int s3k_mem_pmp_get(s3k_index_t i, s3k_pmp_slot_t *slot, s3k_mem_perm_t *perm, s3k_pmp_addr_t *addr)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MEM_PMP_GET;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2");
	register s3k_word_t a3 __asm__("a3");
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1), "=r"(a2), "=r"(a3));
	*slot = a1;
	*perm = a2;
	*addr = a3;
	return a0;
}

static inline int s3k_mem_pmp_set(s3k_index_t i, s3k_pmp_slot_t slot, s3k_mem_perm_t perm, s3k_pmp_addr_t addr)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MEM_PMP_SET;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = slot;
	register s3k_word_t a3 __asm__("a3") = ((s3k_word_t)perm << 56) | addr;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3));
	return a0;
}

static inline int s3k_mem_pmp_clear(s3k_index_t i)
{
	return s3k_mem_pmp_set(i, 0, 0, 0);
}

static inline int s3k_tsl_set(s3k_index_t i, bool enabled)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_TSL_SET;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = enabled;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2));
	return a0;
}

static inline int s3k_mon_suspend(s3k_index_t i)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_SUSPEND;
	register s3k_word_t a1 __asm__("a1") = i;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1));
	return a0;
}

static inline int s3k_mon_resume(s3k_index_t i)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_RESUME;
	register s3k_word_t a1 __asm__("a1") = i;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1));
	return a0;
}

static inline int s3k_mon_yield(s3k_index_t i)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_YIELD;
	register s3k_word_t a1 __asm__("a1") = i;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1));
	return a0;
}

static inline int s3k_mon_reg_get(s3k_index_t i, s3k_reg_t reg, s3k_word_t *value)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_REG_GET;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = reg;
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1) : "r"(a2));
	*value = a1;
	return a0;
}

static inline int s3k_mon_reg_set(s3k_index_t i, s3k_reg_t reg, s3k_word_t value)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_REG_SET;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = reg;
	register s3k_word_t a3 __asm__("a3") = value;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3));
	return a0;
}

static inline int s3k_mon_vreg_get(s3k_index_t i, s3k_vreg_t reg, s3k_word_t *value)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_VREG_GET;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = reg;
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1) : "r"(a2));
	*value = a1;
	return a0;
}

static inline int s3k_mon_state_get(s3k_index_t i, s3k_word_t *value)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_STATE_GET;
	register s3k_word_t a1 __asm__("a1") = i;
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1));
	*value = a1;
	return a0;
}


static inline int s3k_mon_vreg_set(s3k_index_t i, s3k_vreg_t reg, s3k_word_t value)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_VREG_SET;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = reg;
	register s3k_word_t a3 __asm__("a3") = value;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3));
	return a0;
}

static inline int s3k_mon_cap_introspect(s3k_index_t i, s3k_capty_t type, s3k_index_t j,
					 s3k_fuel_t offset, void *cap)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_CAP_INTROSPECT;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = type;
	register s3k_word_t a3 __asm__("a3") = j;
	register s3k_word_t a4 __asm__("a4") = offset;
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1), "+r"(a2) : "r"(a3), "r"(a4));
	if (a0 == S3K_SUCCESS) {
		s3k_word_t *ptr = (s3k_word_t *)cap;
		ptr[0] = a1;
		if (type != S3K_CAPTY_MON) {
			ptr[1] = a2;
		}
	}
	return a0;
}

static inline int s3k_mon_mem_introspect(s3k_index_t i, s3k_index_t j, s3k_fuel_t offset, s3k_cap_mem_t *cap)
{
	return s3k_mon_cap_introspect(i, S3K_CAPTY_MEM, j, offset, cap);
}

static inline int s3k_mon_mem_get(s3k_index_t i, s3k_index_t j, s3k_cap_mem_t *cap)
{
	return s3k_mon_mem_introspect(i, j, 0, cap);
}

static inline int s3k_mon_tsl_introspect(s3k_index_t i, s3k_index_t j, s3k_fuel_t offset, s3k_cap_tsl_t *cap)
{
	return s3k_mon_cap_introspect(i, S3K_CAPTY_TSL, j, offset, cap);
}

static inline int s3k_mon_tsl_get(s3k_index_t i, s3k_index_t j, s3k_cap_tsl_t *cap)
{
	return s3k_mon_tsl_introspect(i, j, 0, cap);
}

static inline int s3k_mon_mon_introspect(s3k_index_t i, s3k_index_t j, s3k_fuel_t offset, s3k_cap_mon_t *cap)
{
	return s3k_mon_cap_introspect(i, S3K_CAPTY_MON, j, offset, cap);
}

static inline int s3k_mon_mon_get(s3k_index_t i, s3k_index_t j, s3k_cap_mon_t *cap)
{
	return s3k_mon_mon_introspect(i, j, 0, cap);
}

static inline int s3k_mon_ipc_introspect(s3k_index_t i, s3k_index_t j, s3k_fuel_t offset, s3k_cap_ipc_t *cap)
{
	return s3k_mon_cap_introspect(i, S3K_CAPTY_IPC, j, offset, cap);
}

static inline int s3k_mon_ipc_get(s3k_index_t i, s3k_index_t j, s3k_cap_ipc_t *cap)
{
	return s3k_mon_ipc_introspect(i, j, 0, cap);
}

static inline int s3k_mon_grant(s3k_index_t i, s3k_capty_t type, s3k_index_t j)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_GRANT;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = type;
	register s3k_word_t a3 __asm__("a3") = j;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3));
	return a0;
}

static inline int s3k_mon_mem_grant(s3k_index_t i, s3k_index_t j)
{
	return s3k_mon_grant(i, S3K_CAPTY_MEM, j);
}

static inline int s3k_mon_tsl_grant(s3k_index_t i, s3k_index_t j)
{
	return s3k_mon_grant(i, S3K_CAPTY_TSL, j);
}

static inline int s3k_mon_mon_grant(s3k_index_t i, s3k_index_t j)
{
	return s3k_mon_grant(i, S3K_CAPTY_MON, j);
}

static inline int s3k_mon_ipc_grant(s3k_index_t i, s3k_index_t j)
{
	return s3k_mon_grant(i, S3K_CAPTY_IPC, j);
}

static inline int s3k_mon_cap_derive(s3k_index_t i, s3k_capty_t type, s3k_index_t j, s3k_fuel_t csize,
				   s3k_word_t option0, s3k_word_t option1, s3k_word_t option2)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_CAP_DERIVE;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = type;
	register s3k_word_t a3 __asm__("a3") = j;
	register s3k_word_t a4 __asm__("a4") = csize;
	register s3k_word_t a5 __asm__("a5") = option0;
	register s3k_word_t a6 __asm__("a6") = option1;
	register s3k_word_t a7 __asm__("a7") = option2;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7));
	return a0;
}

static inline int s3k_mon_mem_derive(s3k_index_t i, s3k_index_t j, s3k_fuel_t csize, s3k_mem_perm_t perm,
				     s3k_mem_addr_t base, s3k_mem_addr_t size)
{
	return s3k_mon_cap_derive(i, S3K_CAPTY_MEM, j, csize, perm, base, size);
}

static inline int s3k_mon_tsl_derive(s3k_index_t i, s3k_index_t j, s3k_fuel_t csize, bool enabled, s3k_time_slot_t size)
{
	return s3k_mon_cap_derive(i, S3K_CAPTY_TSL, j, csize, enabled, size, 0);
}

static inline int s3k_mon_mon_derive(s3k_index_t i, s3k_index_t j, s3k_fuel_t csize)
{
	return s3k_mon_cap_derive(i, S3K_CAPTY_MON, j, csize, 0, 0, 0);
}

static inline int s3k_mon_ipc_derive(s3k_index_t i, s3k_index_t j, s3k_fuel_t csize, s3k_ipc_mode_t mode,
				     s3k_ipc_flag_t flag)
{
	return s3k_mon_cap_derive(i, S3K_CAPTY_IPC, j, csize, mode, flag, 0);
}

static inline int s3k_mon_mem_pmp_get(s3k_index_t i, s3k_index_t j, s3k_pmp_slot_t *slot, s3k_mem_perm_t *perm,
				      s3k_pmp_addr_t *addr)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_MEM_PMP_GET;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = j;
	register s3k_word_t a3 __asm__("a3");
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1), "+r"(a2), "=r"(a3));
	*slot = a1;
	*perm = a2;
	*addr = a3;
	return a0;
}

static inline int s3k_mon_mem_pmp_set(s3k_index_t i, s3k_index_t j, s3k_pmp_slot_t slot, s3k_mem_perm_t perm,
				      s3k_pmp_addr_t addr)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_MEM_PMP_SET;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = j;
	register s3k_word_t a3 __asm__("a3") = slot;
	register s3k_word_t a4 __asm__("a4") = ((s3k_word_t)perm << 56) | addr;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4));
	return a0;
}

static inline int s3k_mon_mem_pmp_clear(s3k_index_t i, s3k_index_t j)
{
	return s3k_mon_mem_pmp_set(i, j, 0, 0, 0);
}

static inline int s3k_mon_tsl_set(s3k_index_t i, s3k_index_t j, bool enabled)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_MON_TSL_SET;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = j;
	register s3k_word_t a3 __asm__("a3") = enabled;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3));
	return a0;
}

static inline int s3k_ipc_send(s3k_index_t i, s3k_msg_t *msg)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_IPC_SEND;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = msg->data[0];
	register s3k_word_t a3 __asm__("a3") = msg->data[1];
	register s3k_word_t a4 __asm__("a4") = msg->capty;
	register s3k_word_t a5 __asm__("a5") = msg->capidx;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5));
	return a0;
}

static inline int s3k_ipc_recv(s3k_index_t i, s3k_msg_t *msg)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_IPC_RECV;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = msg->servtime;
	register s3k_word_t a3 __asm__("a3");
	register s3k_word_t a4 __asm__("a4");
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1), "+r"(a2), "=r"(a3), "=r"(a4));
	if (a0 == S3K_SUCCESS) {
		msg->data[0] = a1;
		msg->data[1] = a2;
		msg->capty = (s3k_capty_t)a3;
		msg->capidx = (s3k_index_t)a4;
	}
	return a0;
}

static inline int s3k_ipc_call(s3k_index_t i, s3k_msg_t *msg)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_IPC_CALL;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = msg->data[0];
	register s3k_word_t a3 __asm__("a3") = msg->data[1];
	register s3k_word_t a4 __asm__("a4") = msg->capty;
	register s3k_word_t a5 __asm__("a5") = msg->capidx;
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1), "+r"(a2), "+r"(a3), "+r"(a4) : "r"(a5));
	if (a0 == S3K_SUCCESS) {
		msg->data[0] = a1;
		msg->data[1] = a2;
		msg->capty = (s3k_capty_t)a3;
		msg->capidx = (s3k_index_t)a4;
	}
	return a0;
}

static inline int s3k_ipc_reply(s3k_index_t i, s3k_msg_t *msg)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_IPC_REPLY;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = msg->data[0];
	register s3k_word_t a3 __asm__("a3") = msg->data[1];
	register s3k_word_t a4 __asm__("a4") = msg->capty;
	register s3k_word_t a5 __asm__("a5") = msg->capidx;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5));
	return a0;
}

static inline int s3k_ipc_replyrecv(s3k_index_t i, s3k_msg_t *msg)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_IPC_REPLYRECV;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = msg->data[0];
	register s3k_word_t a3 __asm__("a3") = msg->data[1];
	register s3k_word_t a4 __asm__("a4") = msg->capty;
	register s3k_word_t a5 __asm__("a5") = msg->capidx;
	register s3k_word_t a6 __asm__("a6") = msg->servtime;
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1), "+r"(a2), "+r"(a3), "+r"(a4) : "r"(a5), "r"(a6));
	if (a0 == S3K_SUCCESS) {
		msg->data[0] = a1;
		msg->data[1] = a2;
		msg->capty = (s3k_capty_t)a3;
		msg->capidx = (s3k_index_t)a4;
	}
	return a0;
}

static inline int s3k_ipc_asend(s3k_index_t i, s3k_word_t msg)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_IPC_ASEND;
	register s3k_word_t a1 __asm__("a1") = i;
	register s3k_word_t a2 __asm__("a2") = msg;
	__asm__ volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2));
	return a0;
}

static inline int s3k_ipc_arecv(s3k_index_t i, s3k_word_t *msg)
{
	register s3k_word_t a0 __asm__("a0") = S3K_SYSCALL_IPC_ARECV;
	register s3k_word_t a1 __asm__("a1") = i;
	__asm__ volatile("ecall" : "+r"(a0), "+r"(a1));
	*msg = a1;
	return a0;
}

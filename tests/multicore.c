#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "exception.h"
#include "ipc.h"
#include "mem.h"
#include "mon.h"
#include "syscall.h"
#include "tsl.h"

// Inspect schedule boundaries and exercise the real ticket lock.
#include "../kern/src/lock.c"
#include "../kern/src/sched.c"
#include "../kern/src/syscall.c"

void test_thread_start(void (*worker)(void));
void test_thread_join(void);
void test_thread_yield(void);

proc_t *test_current;
word_t test_mip;
hart_t test_hart;

static uint64_t now;
static uint64_t deadlines[NUM_HARTS];
static unsigned fences;
static atomic_uint iteration;
static atomic_uint completed;
static unsigned order[2];
static unsigned order_count;

enum { ROUNDS = 100000 };

uint64_t rtc_get_time(void)
{
	return now;
}

void rtc_set_time(uint64_t time)
{
	now = time;
}

uint64_t rtc_get_timeout(word_t hart)
{
	return deadlines[hart];
}

void rtc_set_timeout(word_t hart, uint64_t timeout)
{
	deadlines[hart] = timeout;
}

void temporal_fence(void)
{
	assert(__atomic_load_n(&next_ticket, __ATOMIC_RELAXED)
	       == __atomic_load_n(&serving_ticket, __ATOMIC_RELAXED));
	++fences;
}

static void reset(void)
{
	lock_init();
	for (pid_t pid = 1; pid <= MAX_PID; ++pid) {
		*proc_get(pid) = (proc_t){0};
	}
	proc_init(0);
	sched_init();
	tsl_init();
	mon_init();
	ipc_init();
	mem_t caps[NUM_MEMORY_CAPS] = {
		{.base = 0x80000000, .size = 0x10000000, .rwx = MEM_PERM_RWX},
		{.base = 0x10000000, .size = 0x20, .rwx = MEM_PERM_RW},
	};
	mem_init(caps);
	test_current = proc_get(1);
	test_mip = 0;
	test_hart = 0;
	test_current->timeout = 0;
	proc_get(2)->timeout = 0;
}

static void test_scheduler(void)
{
	reset();
	lock_acquire();
	assert(proc_acquire(1));
	lock_release();
	assert(sched() == 0);
	assert(test_current->timeout == 0);
	assert(deadlines[0] == TIME_SLOT_TICKS);

	lock_acquire();
	proc_release(1);
	test_current->timeout = 100 * TIME_SLOT_TICKS;
	lock_release();
	assert(sched() == 0);
	assert(test_current->timeout == 100 * TIME_SLOT_TICKS);

	test_current->timeout = 0;
	assert(sched() == proc_reload_result(test_current));
	assert(test_current->timeout == MAX_TIME_SLOT * TIME_SLOT_TICKS);
	assert(deadlines[0] == MAX_TIME_SLOT * TIME_SLOT_TICKS);
	assert(sched_yield() == 0);
	assert(test_current->state == PROC_STATE_READY);

	now = 3 * TIME_SLOT_TICKS;
	assert(sched() == 0);
	assert(deadlines[0] == 4 * TIME_SLOT_TICKS);
	now = MAX_TIME_SLOT * TIME_SLOT_TICKS;
	assert(sched() == proc_reload_result(test_current));
	assert(deadlines[0] == 2 * MAX_TIME_SLOT * TIME_SLOT_TICKS);
	sched_yield();

	test_hart = 1;
	assert(sched() == 0);
	assert(deadlines[1] == (MAX_TIME_SLOT + 1) * TIME_SLOT_TICKS);
	now = deadlines[1];
	test_mip = CSR_MIP_MTIP;
	unsigned before = fences;
	sched_wait();
	assert(fences == before + 1);
}

static void queued_worker(void)
{
	lock_acquire();
	order[order_count++] = 1;
	lock_release();
}

static void test_ticket_lock(void)
{
	reset();
	next_ticket = serving_ticket = (word_t)-1;
	order_count = 0;
	lock_acquire();
	assert(!lock_contended());
	test_thread_start(queued_worker);
	while (!lock_contended()) {
		test_thread_yield();
	}
	lock_release();
	lock_acquire();
	order[order_count++] = 2;
	lock_release();
	test_thread_join();
	assert(order_count == 2);
	assert(order[0] == 1 && order[1] == 2);
	assert(next_ticket == serving_ticket);
}

static void suspend_worker(void)
{
	for (unsigned i = 1; i <= ROUNDS; ++i) {
		while (atomic_load_explicit(&iteration, memory_order_acquire) != i) {
			test_thread_yield();
		}
		lock_acquire();
		proc_suspend(1);
		lock_release();
		atomic_store_explicit(&completed, i, memory_order_release);
	}
}

static void test_suspend_release(void)
{
	reset();
	test_thread_start(suspend_worker);
	for (unsigned i = 1; i <= ROUNDS; ++i) {
		lock_acquire();
		assert(proc_acquire(1));
		lock_release();
		atomic_store_explicit(&iteration, i, memory_order_release);
		lock_acquire();
		proc_release(1);
		lock_release();
		while (atomic_load_explicit(&completed, memory_order_acquire) != i) {
			test_thread_yield();
		}
		lock_acquire();
		proc_resume(1);
		assert(test_current->state == PROC_STATE_READY);
		lock_release();
	}
	test_thread_join();
}

static void test_lazy_pmp(void)
{
	reset();
	lock_acquire();
	assert(proc_acquire(1));
	word_t config = ((word_t)MEM_PERM_RWX << 56) | 0x200000ff;
	assert(mem_pmp_set(1, 0, 1, config) == 0);
	word_t addr = test_current->pmp.addr[0];
	assert(proc_pmp_is_set(1, 0));
	assert(mem_pmp_set(1, 0, 0, ~(word_t)0) == 0);
	assert(!proc_pmp_is_set(1, 0));
	assert(test_current->pmp.addr[0] == addr);
	assert(mem_pmp_set(1, 0, 1, config) == 0);

	test_current = proc_get(2);
	proc_resume(2);
	assert(proc_acquire(2));
	assert(mem_pmp_set(1, 0, 2, config) == ERR_INVALID_STATE);
	assert(mem_pmp_set(1, 0, 0, ~(word_t)0) == 0);
	assert(mem_pmp_set(1, 0, 0, 0) == 0);
	assert(mem_pmp_set(1, UINT16_MAX, 0, 0) == ERR_INVALID_ACCESS);
	assert(proc_get(1)->pmp.addr[0] == addr);
	assert(proc_get(1)->state == PROC_STATE_ACQUIRED);
	assert(proc_get(1)->pmp.cfg[0] == 0);
	proc_release(1);
	assert(mem_pmp_set(1, 0, 1, config) == 0);
	lock_release();
}

static void test_revocation(void)
{
	reset();
	lock_acquire();
	int a = tsl_derive(1, 0, 1, 4, true, 8);
	int b = tsl_derive(1, 0, 1, 4, true, 8);
	assert(a >= 0 && b >= 0);
	assert(tsl_derive(1, b, 2, 1, true, 4) >= 0);
	order_count = 0;
	test_thread_start(queued_worker);
	while (!lock_contended()) {
		test_thread_yield();
	}
	assert(tsl_revoke(1, 0) > 0);
	assert(schedule[0][0].length == 20);
	assert(schedule[0][16].length == 0);
	assert(schedule[0][20].pid == 2 && schedule[0][20].length == 4);
	assert(schedule[0][24].length == 8);
	lock_release();
	test_thread_join();

	lock_acquire();
	assert(tsl_revoke(1, 0) == 0);
	assert(schedule[0][0].length == MAX_TIME_SLOT);
	for (time_slot_t i = 1; i < MAX_TIME_SLOT; ++i) {
		assert(schedule[0][i].pid == 0 && schedule[0][i].length == 0);
	}
	assert(tsl_derive(1, 0, 2, 1, true, MAX_TIME_SLOT) >= 0);
	assert(schedule[0][0].pid == 2);
	assert(tsl_revoke(1, 0) == 0);
	assert(schedule[0][0].pid == 1 && schedule[0][0].length == MAX_TIME_SLOT);

	int child = tsl_derive(1, 0, 1, 4, true, 8);
	assert(child >= 0);
	assert(tsl_derive(1, child, 2, 1, true, 8) >= 0);
	test_mip = CSR_MIP_MTIP;
	assert(tsl_revoke(1, 0) > 0);
	assert(schedule[0][24].pid == 2 && schedule[0][24].length == 8);
	test_mip = 0;
	assert(tsl_revoke(1, 0) == 0);
	assert(schedule[0][24].pid == 0 && schedule[0][24].length == 0);
	lock_release();
}

static void test_revocation_progress(void)
{
	for (unsigned timer = 0; timer < 2; ++timer) {
		reset();
		lock_acquire();
		assert(mem_derive(1, 0, 1, 1, MEM_PERM_R, 0x80000000, 0x1000) >= 0);
		assert(mem_derive(1, 0, 1, 1, MEM_PERM_R, 0x80001000, 0x1000) >= 0);
		assert(mon_derive(1, 0, 1, 1) >= 0);
		assert(mon_derive(1, 0, 1, 1) >= 0);
		assert(ipc_derive(1, 0, 1, 1, IPC_MODE_USYNC, 0) >= 0);
		assert(ipc_derive(1, 0, 1, 1, IPC_MODE_USYNC, 0) >= 0);
		assert(tsl_derive(1, 0, 1, 1, true, 4) >= 0);
		assert(tsl_derive(1, 0, 1, 1, true, 4) >= 0);
		if (timer) {
			test_mip = CSR_MIP_MTIP;
		} else {
			order_count = 0;
			test_thread_start(queued_worker);
			while (!lock_contended()) {
				test_thread_yield();
			}
		}
		assert(mem_revoke(1, 0) == 1);
		assert(mon_revoke(1, 0) == 1);
		assert(ipc_revoke(1, 0) == 1);
		assert(tsl_revoke(1, 0) == 1);
		lock_release();
		if (!timer) {
			test_thread_join();
		}
		test_mip = 0;
		lock_acquire();
		assert(mem_revoke(1, 0) == 0);
		assert(mon_revoke(1, 0) == 0);
		assert(ipc_revoke(1, 0) == 0);
		assert(tsl_revoke(1, 0) == 0);
		lock_release();
	}
}

static proc_result_t syscall_number(word_t nr)
{
	test_current->regs.a0 = nr;
	return syscall_handler();
}

static proc_result_t invoke(handler_t handler)
{
	for (word_t nr = 0; nr < ARRAY_SIZE(handlers); ++nr) {
		if (handlers[nr] == handler) {
			return syscall_number(nr);
		}
	}
	assert(false);
	return 0;
}

static void test_process_result_encoding(void)
{
	reset();
	for (pid_t pid = 1; pid <= MAX_PID; ++pid) {
		proc_t *proc = proc_get(pid);
		uintptr_t address = (uintptr_t)proc;
		assert(address != 0 && address <= INTPTR_MAX);
		proc_result_t resume = proc_resume_result(proc);
		proc_result_t reload = proc_reload_result(proc);
		assert(resume > 0);
		assert(reload < 0);
		assert(resume == (proc_result_t)address);
		assert(reload == -resume);
		assert((uintptr_t)0 - (uintptr_t)reload == address);
	}
}

static void test_trap_results(void)
{
	reset();
	assert(sched() == proc_reload_result(test_current));
	assert(invoke(syscall_pid_get) == proc_resume_result(test_current));
	assert(test_current->state == PROC_STATE_ACQUIRED);
	test_current->regs.a1 = 0;
	test_current->regs.a2 = 1;
	test_current->regs.a3 = ((word_t)MEM_PERM_RWX << 56) | 0x200000ff;
	assert(invoke(syscall_mem_pmp_set) == proc_resume_result(test_current));
	assert(test_current->pmp.cfg[0] != 0);
	test_current->regs.a2 = 0;
	assert(invoke(syscall_mem_pmp_set) == proc_resume_result(test_current));
	assert(test_current->pmp.cfg[0] == 0);
	uint64_t timeout = test_current->timeout;
	assert(invoke(syscall_sync) == proc_reload_result(test_current));
	assert(test_current->state == PROC_STATE_ACQUIRED);
	assert(test_current->timeout == timeout);
	assert(deadlines[0] == timeout);
	test_current->regs.a1 = 0;
	assert(invoke(syscall_sleep_until) == 0);
	assert(test_current->state == PROC_STATE_READY);

	test_current->timeout = 0;
	assert(sched() == proc_reload_result(test_current));
	word_t pc = test_current->regs.pc;
	test_mip = CSR_MIP_MTIP;
	assert(invoke(syscall_pid_get) == 0);
	assert(test_current->regs.pc == pc);
	assert(test_current->state == PROC_STATE_READY);

	test_mip = 0;
	test_current->timeout = 0;
	assert(sched() == proc_reload_result(test_current));
	test_current->trap.tpc = 0x1234;
	assert(exception_handler(2, 0) == proc_resume_result(test_current));
	assert(test_current->regs.pc == 0x1234);
	assert(test_current->state == PROC_STATE_ACQUIRED);
	assert(syscall_number(ARRAY_SIZE(handlers)) == proc_resume_result(test_current));
	assert(test_current->trap.ecause == 8);

	lock_acquire();
	proc_resume(2);
	lock_release();
	test_current->regs.a1 = MAX_MONITOR_FUEL;
	assert(invoke(syscall_mon_yield) == proc_reload_result(proc_get(2)));
	assert(proc_get(1)->state == PROC_STATE_READY);
	assert(proc_get(2)->state == PROC_STATE_ACQUIRED);
	assert(proc_get(2)->timeout == proc_get(1)->timeout);
}

static void test_pmp_reload_policy(void)
{
	reset();
	assert(sched() == proc_reload_result(test_current));
	const handler_t ordinary[] = {
		syscall_cap_revoke, syscall_cap_delete, syscall_mem_pmp_set,
		syscall_mon_grant, syscall_mon_mem_pmp_set,
		syscall_ipc_send, syscall_ipc_call, syscall_ipc_reply, syscall_ipc_replyrecv,
	};
	uint64_t timeout = test_current->timeout;
	for (size_t i = 0; i < ARRAY_SIZE(ordinary); ++i) {
		test_current->regs.a1 = UINT16_MAX;
		assert(invoke(ordinary[i]) == proc_resume_result(test_current));
		assert(test_current->state == PROC_STATE_ACQUIRED);
		assert(test_current->timeout == timeout);
	}

	test_current->regs.a1 = 0;
	test_current->regs.a2 = 1;
	test_current->regs.a3 = ((word_t)MEM_PERM_RWX << 56) | 0x200000ff;
	assert(invoke(syscall_mem_pmp_set) == proc_resume_result(test_current));
	assert(test_current->regs.a0 == ERR_SUCCESS);
	word_t addr = test_current->pmp.addr[0];
	test_current->regs.a1 = MAX_MONITOR_FUEL;
	test_current->regs.a2 = CAPTY_MEM;
	test_current->regs.a3 = 0;
	assert(invoke(syscall_mon_grant) == proc_resume_result(test_current));
	assert(test_current->regs.a0 == ERR_SUCCESS);
	assert(test_current->pmp.cfg[0] == 0);
	assert(test_current->pmp.addr[0] == addr);
	assert(mem_valid_access(2, 0));
	assert(invoke(syscall_sync) == proc_reload_result(test_current));
	assert(test_current->state == PROC_STATE_ACQUIRED);
	assert(test_current->timeout == timeout);
}

static int expected_introspection(pid_t owner, capty_t type, fuel_t offset, word_t words[2])
{
	switch (type) {
	case CAPTY_MEM:
		return mem_introspect(owner, 0, offset, (mem_t *)words);
	case CAPTY_TSL:
		return tsl_introspect(owner, 0, offset, (tsl_t *)words);
	case CAPTY_MON:
		return mon_introspect(owner, 0, offset, (mon_t *)words);
	case CAPTY_IPC:
		return ipc_introspect(owner, 0, offset, (ipc_t *)words);
	default:
		assert(false);
		return ERR_INVALID_ARGUMENT;
	}
}

static void test_capability_syscalls(void)
{
	const struct {
		capty_t type;
		int (*revoke)(pid_t, index_t);
		int (*delete)(pid_t, index_t);
	} cases[] = {
		{CAPTY_MEM, mem_revoke, mem_delete},
		{CAPTY_TSL, tsl_revoke, tsl_delete},
		{CAPTY_MON, mon_revoke, mon_delete},
		{CAPTY_IPC, ipc_revoke, ipc_delete},
	};
	for (size_t c = 0; c < ARRAY_SIZE(cases); ++c) {
		capty_t type = cases[c].type;
		for (unsigned remote = 0; remote < 2; ++remote) {
			reset();
			assert(sched() == proc_reload_result(test_current));
			lock_acquire();
			if (remote) {
				assert(mon_grant(1, MAX_MONITOR_FUEL, type, 0) == ERR_SUCCESS);
			}
			lock_release();
			for (unsigned offset_case = 0; offset_case < 3; ++offset_case) {
				fuel_t offset = offset_case == 2 ? UINT16_MAX : offset_case;
				word_t expected[2] = {0, 0};
				lock_acquire();
				int status = expected_introspection(remote ? 2 : 1, type, offset, expected);
				lock_release();
				test_current->regs.a1 = remote ? MAX_MONITOR_FUEL : type;
				test_current->regs.a2 = remote ? type : 0;
				test_current->regs.a3 = remote ? 0 : offset;
				test_current->regs.a4 = offset;
				assert(invoke(remote ? syscall_mon_cap_introspect : syscall_cap_introspect)
				       == proc_resume_result(test_current));
				assert(test_current->regs.a0 == (word_t)status);
				if (status == ERR_SUCCESS) {
					size_t size = type == CAPTY_MON ? sizeof(mon_t) : sizeof(expected);
					assert(memcmp(&test_current->regs.a1, expected, size) == 0);
					if (type == CAPTY_MON) {
						assert(test_current->regs.a2 == (remote ? type : 0));
					}
				}
			}
			test_current->regs.a1 = remote ? UINT16_MAX : type;
			test_current->regs.a2 = remote ? type : UINT16_MAX;
			test_current->regs.a3 = 0;
			test_current->regs.a4 = 0;
			assert(invoke(remote ? syscall_mon_cap_introspect : syscall_cap_introspect)
			       == proc_resume_result(test_current));
			assert(test_current->regs.a0 == (word_t)ERR_INVALID_ACCESS);
			if (remote) {
				test_current->regs.a1 = MAX_MONITOR_FUEL;
				test_current->regs.a2 = type;
				test_current->regs.a3 = MAX_MONITOR_FUEL;
				assert(invoke(syscall_mon_cap_introspect) == proc_resume_result(test_current));
				assert(test_current->regs.a0 == (word_t)ERR_INVALID_ACCESS);
			}
		}
		for (unsigned deletion = 0; deletion < 2; ++deletion) {
			int (*operation)(pid_t, index_t) = deletion ? cases[c].delete : cases[c].revoke;
			reset();
			lock_acquire();
			int expected = operation(1, 0);
			lock_release();
			reset();
			assert(sched() == proc_reload_result(test_current));
			test_current->regs.a1 = type;
			test_current->regs.a2 = 0;
			assert(invoke(deletion ? syscall_cap_delete : syscall_cap_revoke)
			       == proc_resume_result(test_current));
			assert(test_current->regs.a0 == (word_t)expected);
			assert(test_current->state == PROC_STATE_ACQUIRED);
			test_current->regs.a1 = type;
			test_current->regs.a2 = UINT16_MAX;
			assert(invoke(deletion ? syscall_cap_delete : syscall_cap_revoke)
			       == proc_resume_result(test_current));
			assert(test_current->regs.a0 == (word_t)ERR_INVALID_ACCESS);
		}
	}
	const word_t invalid_types[] = {CAPTY_NONE, CAPTY_IPC + 1, ((word_t)1 << 32) | CAPTY_MEM};
	for (size_t i = 0; i < ARRAY_SIZE(invalid_types); ++i) {
		const handler_t operations[] = {
			syscall_cap_introspect, syscall_mon_cap_introspect,
			syscall_cap_revoke, syscall_cap_delete,
		};
		for (size_t j = 0; j < ARRAY_SIZE(operations); ++j) {
			reset();
			assert(sched() == proc_reload_result(test_current));
			bool remote = operations[j] == syscall_mon_cap_introspect;
			test_current->regs.a1 = remote ? MAX_MONITOR_FUEL : invalid_types[i];
			test_current->regs.a2 = remote ? invalid_types[i] : 0;
			test_current->regs.a3 = 0;
			test_current->regs.a4 = 0;
			assert(invoke(operations[j]) == proc_resume_result(test_current));
			assert(test_current->regs.a0 == (word_t)ERR_INVALID_ARGUMENT);
			assert(mem_valid_access(1, 0) && tsl_valid_access(1, 0));
			assert(mon_valid_access(1, 0) && ipc_valid_access(1, 0));
		}
	}
}

static int expected_derivation(pid_t target, capty_t type)
{
	switch (type) {
	case CAPTY_MEM:
		return mem_derive(1, 0, target, 1, MEM_PERM_RW, 0x80000000, 0x1000);
	case CAPTY_TSL:
		return tsl_derive(1, 0, target, 1, false, 1);
	case CAPTY_MON:
		return mon_derive(1, 0, target, 1);
	case CAPTY_IPC:
		return ipc_derive(1, 0, target, 1, IPC_MODE_ASYNC, 0);
	default:
		assert(false);
		return ERR_INVALID_ARGUMENT;
	}
}

static void test_derive_syscalls(void)
{
	for (capty_t type = CAPTY_MEM; type <= CAPTY_IPC; ++type) {
		for (unsigned remote = 0; remote < 2; ++remote) {
			pid_t target = remote ? 2 : 1;
			reset();
			lock_acquire();
			int expected = expected_derivation(target, type);
			assert(expected >= 0);
			word_t expected_cap[2] = {0, 0};
			assert(cap_introspect(target, type, expected, 0, expected_cap) == ERR_SUCCESS);
			lock_release();
			reset();
			assert(sched() == proc_reload_result(test_current));
			uint64_t timeout = test_current->timeout;
			unsigned first = remote ? 4 : 3;
			test_current->regs.a1 = remote ? MAX_MONITOR_FUEL : type;
			test_current->regs.a2 = remote ? type : 0;
			test_current->regs.a3 = remote ? 0 : 1;
			word_t *args = &test_current->regs.a0;
			args[first] = 1;
			args[first + 1] = type == CAPTY_MEM ? MEM_PERM_RW : type == CAPTY_IPC ? IPC_MODE_ASYNC : 0;
			args[first + 2] = type == CAPTY_MEM ? 0x80000000 : type == CAPTY_TSL ? 1 : 0;
			args[first + 3] = type == CAPTY_MEM ? 0x1000 : 0;
			handler_t handler = remote ? syscall_mon_cap_derive : syscall_cap_derive;
			assert(invoke(handler) == proc_resume_result(test_current));
			assert(args[0] == (word_t)expected);
			assert(test_current->state == PROC_STATE_ACQUIRED);
			assert(test_current->timeout == timeout);
			word_t actual_cap[2] = {0, 0};
			lock_acquire();
			assert(cap_introspect(target, type, expected, 0, actual_cap) == ERR_SUCCESS);
			lock_release();
			assert(memcmp(actual_cap, expected_cap, type == CAPTY_MON ? sizeof(mon_t) : sizeof(actual_cap)) == 0);
			args[first] = UINT16_MAX;
			assert(invoke(handler) == proc_resume_result(test_current));
			assert(args[0] == (word_t)ERR_INVALID_ARGUMENT);
			args[first] = 1;
			args[remote ? 3 : 2] = UINT16_MAX;
			assert(invoke(handler) == proc_resume_result(test_current));
			assert(args[0] == (word_t)ERR_INVALID_ACCESS);
			if (remote) {
				args[1] = UINT16_MAX;
				assert(invoke(handler) == proc_resume_result(test_current));
				assert(args[0] == (word_t)ERR_INVALID_ACCESS);
			}
		}
	}
	const word_t invalid[] = {CAPTY_NONE, CAPTY_IPC + 1, ((word_t)1 << 32) | CAPTY_MEM};
	for (size_t i = 0; i < ARRAY_SIZE(invalid); ++i) {
		for (unsigned remote = 0; remote < 2; ++remote) {
			reset();
			assert(sched() == proc_reload_result(test_current));
			test_current->regs.a1 = remote ? MAX_MONITOR_FUEL : invalid[i];
			test_current->regs.a2 = remote ? invalid[i] : 0;
			test_current->regs.a3 = remote ? 0 : 1;
			test_current->regs.a4 = 1;
			assert(invoke(remote ? syscall_mon_cap_derive : syscall_cap_derive)
			       == proc_resume_result(test_current));
			assert(test_current->regs.a0 == (word_t)ERR_INVALID_ARGUMENT);
		}
	}
}

static void test_monitor_grant(void)
{
	for (capty_t type = CAPTY_MEM; type <= CAPTY_IPC; ++type) {
		reset();
		assert(sched() == proc_reload_result(test_current));
		uint64_t timeout = test_current->timeout;
		lock_acquire();
		assert(mon_grant(1, UINT16_MAX, type, 0) == ERR_INVALID_ACCESS);
		assert(mon_grant(2, MAX_MONITOR_FUEL, type, 0) == ERR_INVALID_ACCESS);
		assert(mon_grant(1, MAX_MONITOR_FUEL, type, UINT16_MAX) == ERR_INVALID_ACCESS);
		assert(mon_grant(1, MAX_MONITOR_FUEL, CAPTY_NONE, 0) == ERR_INVALID_ARGUMENT);
		assert(mon_grant(1, MAX_MONITOR_FUEL, (capty_t)255, 0) == ERR_INVALID_ARGUMENT);
		lock_release();
		test_current->regs.a1 = MAX_MONITOR_FUEL;
		test_current->regs.a2 = ((word_t)1 << 32) | type;
		test_current->regs.a3 = 0;
		assert(invoke(syscall_mon_grant) == proc_resume_result(test_current));
		assert(test_current->regs.a0 == (word_t)ERR_INVALID_ARGUMENT);
		test_current->regs.a2 = type;
		assert(invoke(syscall_mon_grant) == proc_resume_result(test_current));
		assert(test_current->regs.a0 == ERR_SUCCESS);
		assert(test_current->state == PROC_STATE_ACQUIRED);
		assert(test_current->timeout == timeout);
		switch (type) {
		case CAPTY_MEM:
			assert(!mem_valid_access(1, 0) && mem_valid_access(2, 0));
			break;
		case CAPTY_TSL:
			assert(!tsl_valid_access(1, 0) && tsl_valid_access(2, 0));
			assert(schedule[0][0].pid == 2);
			break;
		case CAPTY_MON:
			assert(!mon_valid_access(1, 0) && mon_valid_access(2, 0));
			break;
		case CAPTY_IPC:
			assert(!ipc_valid_access(1, 0) && ipc_valid_access(2, 0));
			break;
		default:
			assert(false);
		}
		lock_acquire();
		assert(mon_grant(1, MAX_MONITOR_FUEL, type, 0) == ERR_INVALID_ACCESS);
		lock_release();
	}
}

static index_t setup_channel_mode(ipc_mode_t mode, ipc_flag_t flags, index_t *sink)
{
	reset();
	lock_acquire();
	int i = ipc_derive(1, 0, 2, 4, mode, flags);
	assert(i >= 0);
	*sink = i;
	int source = ipc_derive(2, *sink, 1, 1, mode, flags);
	assert(source >= 0);
	proc_resume(2);
	assert(proc_acquire(2));
	test_current = proc_get(2);
	lock_release();
	test_current->regs.a1 = *sink;
	test_current->regs.a2 = 0;
	assert(invoke(syscall_ipc_recv) == 0);
	assert(test_current->state & PROC_STATE_BLOCKED);
	assert(!(test_current->state & PROC_STATE_ACQUIRED));

	lock_acquire();
	test_current = proc_get(1);
	assert(proc_acquire(1));
	test_current->timeout = 1000 * TIME_SLOT_TICKS;
	lock_release();
	test_current->regs.a1 = source;
	test_current->regs.a2 = 11;
	test_current->regs.a3 = 22;
	test_current->regs.a4 = CAPTY_NONE;
	test_current->regs.a5 = 0;
	return source;
}

static index_t setup_channel(ipc_flag_t flags, index_t *sink)
{
	return setup_channel_mode(IPC_MODE_BSYNC, flags, sink);
}

static void test_ipc_handoffs(void)
{
	index_t sink;
	for (unsigned replyrecv = 0; replyrecv < 2; ++replyrecv) {
		setup_channel(IPC_FLAG_YIELD, &sink);
		assert(invoke(syscall_ipc_call) == proc_reload_result(proc_get(2)));
		assert(proc_get(1)->state & PROC_STATE_BLOCKED);
		assert(!(proc_get(1)->state & PROC_STATE_ACQUIRED));
		test_current = proc_get(2);
		assert(test_current->state == PROC_STATE_ACQUIRED);
		assert(test_current->regs.a1 == 11 && test_current->regs.a2 == 22);
		test_current->regs.a1 = sink;
		test_current->regs.a2 = 33;
		test_current->regs.a3 = 44;
		test_current->regs.a4 = CAPTY_NONE;
		test_current->regs.a5 = 0;
		test_current->regs.a6 = 0;
		assert(invoke(replyrecv ? syscall_ipc_replyrecv : syscall_ipc_reply)
		       == proc_reload_result(proc_get(1)));
		assert(!(proc_get(2)->state & PROC_STATE_ACQUIRED));
		assert(proc_get(1)->state == PROC_STATE_ACQUIRED);
		assert(proc_get(1)->regs.a1 == 33 && proc_get(1)->regs.a2 == 44);
		assert(proc_get(1)->timeout == 1000 * TIME_SLOT_TICKS);
	}

	setup_channel(0, &sink);
	assert(invoke(syscall_ipc_call) == 0);
	assert(proc_get(1)->state & PROC_STATE_BLOCKED);
	assert(!(proc_get(1)->state & PROC_STATE_ACQUIRED));
	assert(proc_get(2)->state == PROC_STATE_READY);
	assert(proc_get(2)->timeout == 0);
	lock_acquire();
	sched_set_pid(1, 2, 0);
	lock_release();
	test_hart = 1;
	assert(sched() == proc_reload_result(proc_get(2)));
}

static proc_result_t invoke_direct(handler_t handler)
{
	lock_acquire();
	proc_result_t result = handler(test_current->pid, &test_current->regs.a0);
	lock_release();
	return result;
}

static void test_individual_handler_results(void)
{
	reset();
	assert(sched() == proc_reload_result(test_current));
	uint64_t timeout = test_current->timeout;
	assert(invoke_direct(syscall_pid_get) == proc_resume_result(test_current));
	assert(invoke_direct(syscall_sync) == proc_reload_result(test_current));
	assert(test_current->state == PROC_STATE_ACQUIRED);
	assert(test_current->timeout == timeout);
	test_current->regs.a1 = UINT16_MAX;
	const handler_t handoffs[] = {
		syscall_mon_yield, syscall_ipc_send, syscall_ipc_recv, syscall_ipc_call,
		syscall_ipc_reply, syscall_ipc_replyrecv, syscall_ipc_asend,
	};
	for (size_t i = 0; i < ARRAY_SIZE(handoffs); ++i) {
		assert(invoke_direct(handoffs[i]) == proc_resume_result(test_current));
		assert(test_current->state == PROC_STATE_ACQUIRED);
	}
	test_current->regs.a1 = 0;
	assert(invoke_direct(syscall_sleep_until) == 0);
	assert(test_current->state == PROC_STATE_READY);
	assert(test_current->timeout == timeout);

	lock_acquire();
	assert(proc_acquire(1));
	proc_resume(2);
	lock_release();
	test_current->regs.a1 = MAX_MONITOR_FUEL;
	assert(invoke_direct(syscall_mon_yield) == proc_reload_result(proc_get(2)));
	assert(proc_get(1)->state == PROC_STATE_READY);
	assert(proc_get(2)->state == PROC_STATE_ACQUIRED);

	index_t sink;
	for (unsigned replyrecv = 0; replyrecv < 2; ++replyrecv) {
		setup_channel(IPC_FLAG_YIELD, &sink);
		assert(invoke_direct(syscall_ipc_call) == proc_reload_result(proc_get(2)));
		assert(!(proc_get(1)->state & PROC_STATE_ACQUIRED));
		test_current = proc_get(2);
		test_current->regs.a1 = sink;
		test_current->regs.a4 = CAPTY_NONE;
		test_current->regs.a6 = 0;
		assert(invoke_direct(replyrecv ? syscall_ipc_replyrecv : syscall_ipc_reply)
		       == proc_reload_result(proc_get(1)));
		assert(!(proc_get(2)->state & PROC_STATE_ACQUIRED));
		assert(proc_get(1)->state == PROC_STATE_ACQUIRED);
	}

	setup_channel(0, &sink);
	assert(invoke_direct(syscall_ipc_call) == 0);
	assert(proc_get(1)->state & PROC_STATE_BLOCKED);
	assert(!(proc_get(1)->state & PROC_STATE_ACQUIRED));
	assert(proc_get(2)->state == PROC_STATE_READY);

	reset();
	lock_acquire();
	int channel = ipc_derive(1, 0, 1, 4, IPC_MODE_BSYNC, 0);
	assert(channel >= 0);
	assert(proc_acquire(1));
	lock_release();
	test_current->regs.a1 = channel;
	test_current->regs.a2 = 0;
	assert(invoke_direct(syscall_ipc_recv) == 0);
	assert(test_current->state & PROC_STATE_BLOCKED);
	assert(!(test_current->state & PROC_STATE_ACQUIRED));
}

static void test_shared_ipc_transfers(void)
{
	for (capty_t type = CAPTY_MEM; type <= CAPTY_IPC; ++type) {
		for (unsigned call = 0; call < 2; ++call) {
			for (unsigned yielding = 0; yielding < 2; ++yielding) {
				index_t sink;
				ipc_flag_t flags = IPC_FLAG_MASK & (yielding ? IPC_FLAG_MASK : ~IPC_FLAG_YIELD);
				index_t source = setup_channel_mode(call ? IPC_MODE_BSYNC : IPC_MODE_USYNC, flags, &sink);
				lock_acquire();
				word_t address = 0;
				if (type == CAPTY_MEM) {
					assert(mem_pmp_set(1, 0, 1, ((word_t)MEM_PERM_RWX << 56) | 0x200000ff) == ERR_SUCCESS);
					address = test_current->pmp.addr[0];
				}
				word_t data[2] = {77, 88};
				word_t *status = &test_current->regs.a0;
				proc_result_t result = call ? ipc_call(1, source, data, type, 0, status)
							   : ipc_send(1, source, data, type, 0, status);
				assert(result == (yielding ? proc_reload_result(proc_get(2))
							 : call ? 0 : proc_resume_result(proc_get(1))));
				assert(*status == (word_t)(call ? ERR_TIMEOUT : ERR_SUCCESS));
				word_t cap[2] = {0, 0};
				assert(cap_introspect(2, type, 0, 0, cap) == ERR_SUCCESS);
				assert(cap_introspect(1, type, 0, 0, cap) == ERR_INVALID_ACCESS);
				assert(proc_get(2)->regs.a1 == 77 && proc_get(2)->regs.a2 == 88);
				assert(proc_get(2)->regs.a3 == type && proc_get(2)->regs.a4 == 0);
				if (type == CAPTY_MEM) {
					assert(proc_get(1)->pmp.cfg[0] == 0);
					assert(proc_get(1)->pmp.addr[0] == address);
				}
				assert(((proc_get(1)->state & PROC_STATE_BLOCKED) != 0) == (call != 0));
				assert(((proc_get(1)->state & PROC_STATE_ACQUIRED) != 0) == (!call && !yielding));
				lock_release();
			}
		}
	}

	index_t sink;
	index_t source = setup_channel(IPC_FLAG_YIELD, &sink);
	lock_acquire();
	word_t data[2] = {1, 2};
	word_t *status = &test_current->regs.a0;
	assert(ipc_call(1, source, data, CAPTY_MEM, 0, status) == proc_resume_result(test_current));
	assert(*status == (word_t)ERR_INVALID_ARGUMENT);
	assert(mem_valid_access(1, 0));
	assert(proc_get(2)->state & PROC_STATE_BLOCKED);
	assert(test_current->state == PROC_STATE_ACQUIRED);
	lock_release();
}

static void test_shared_pmp_handlers(void)
{
	reset();
	assert(sched() == proc_reload_result(test_current));
	uint64_t timeout = test_current->timeout;
	for (unsigned remote = 0; remote < 2; ++remote) {
		word_t *args = &test_current->regs.a0;
		args[1] = 0;
		if (remote) {
			args[2] = 0;
		}
		assert(invoke(remote ? syscall_mon_mem_pmp_get : syscall_mem_pmp_get)
		       == proc_resume_result(test_current));
		assert(args[0] == ERR_SUCCESS && args[1] == 0 && args[2] == 0 && args[3] == 0);
		args[1] = 0;
		args[2] = remote ? 0 : 1;
		args[3] = remote ? 1 : ((word_t)MEM_PERM_RWX << 56) | 0x200000ff;
		args[4] = ((word_t)MEM_PERM_RWX << 56) | 0x200000ff;
		assert(invoke(remote ? syscall_mon_mem_pmp_set : syscall_mem_pmp_set)
		       == proc_resume_result(test_current));
		assert(args[0] == ERR_SUCCESS);
		args[1] = 0;
		args[2] = 0;
		assert(invoke(remote ? syscall_mon_mem_pmp_get : syscall_mem_pmp_get)
		       == proc_resume_result(test_current));
		assert(args[0] == ERR_SUCCESS && args[1] == 1 && args[2] == MEM_PERM_RWX);
		assert(args[3] == 0x200000ff);
		args[1] = 0;
		args[2] = 0;
		args[3] = 0;
		args[4] = ~(word_t)0;
		assert(invoke(remote ? syscall_mon_mem_pmp_set : syscall_mem_pmp_set)
		       == proc_resume_result(test_current));
		assert(args[0] == ERR_SUCCESS && test_current->pmp.cfg[0] == 0);
		assert(test_current->pmp.addr[0] == 0x200000ff);
		args[1] = remote ? 0 : UINT16_MAX;
		args[2] = UINT16_MAX;
		assert(invoke(remote ? syscall_mon_mem_pmp_get : syscall_mem_pmp_get)
		       == proc_resume_result(test_current));
		assert(args[0] == (word_t)ERR_INVALID_ACCESS);
		assert(args[1] == 0 && args[2] == 0 && args[3] == 0);
	}
	test_current->regs.a1 = UINT16_MAX;
	test_current->regs.a2 = 123;
	assert(invoke(syscall_mon_mem_pmp_get) == proc_resume_result(test_current));
	assert(test_current->regs.a0 == (word_t)ERR_INVALID_ACCESS && test_current->regs.a2 == 123);
	assert(test_current->state == PROC_STATE_ACQUIRED && test_current->timeout == timeout);
}

static void test_operation_handoffs(void)
{
	reset();
	assert(sched() == proc_reload_result(test_current));
	word_t *status = &test_current->regs.a0;
	word_t data[2] = {11, 22};
	lock_acquire();
	assert(mon_yield(1, UINT16_MAX, status) == proc_resume_result(test_current));
	assert(*status == (word_t)ERR_INVALID_ACCESS);
	assert(ipc_send(1, UINT16_MAX, data, CAPTY_NONE, 0, status) == proc_resume_result(test_current));
	assert(*status == (word_t)ERR_INVALID_ACCESS);
	assert(ipc_recv(1, UINT16_MAX, 0, status) == proc_resume_result(test_current));
	assert(*status == (word_t)ERR_INVALID_ACCESS);
	assert(ipc_call(1, UINT16_MAX, data, CAPTY_NONE, 0, status) == proc_resume_result(test_current));
	assert(*status == (word_t)ERR_INVALID_ACCESS);
	assert(ipc_reply(1, UINT16_MAX, data, CAPTY_NONE, 0, status) == proc_resume_result(test_current));
	assert(*status == (word_t)ERR_INVALID_ACCESS);
	assert(ipc_replyrecv(1, UINT16_MAX, data, CAPTY_NONE, 0, 0, status) == proc_resume_result(test_current));
	assert(*status == (word_t)ERR_INVALID_ACCESS);
	assert(ipc_asend(1, UINT16_MAX, 11, status) == proc_resume_result(test_current));
	assert(*status == (word_t)ERR_INVALID_ACCESS);
	assert(test_current->state == PROC_STATE_ACQUIRED);
	proc_resume(2);
	assert(mon_yield(1, MAX_MONITOR_FUEL, status) == proc_reload_result(proc_get(2)));
	assert(*status == ERR_SUCCESS);
	assert(proc_get(1)->state == PROC_STATE_READY);
	assert(proc_get(2)->state == PROC_STATE_ACQUIRED);
	lock_release();

	index_t sink;
	for (unsigned yielding = 0; yielding < 2; ++yielding) {
		index_t source = setup_channel_mode(IPC_MODE_USYNC, yielding ? IPC_FLAG_YIELD : 0, &sink);
		status = &test_current->regs.a0;
		lock_acquire();
		proc_result_t result = ipc_send(1, source, data, CAPTY_NONE, 0, status);
		assert(*status == ERR_SUCCESS);
		assert(proc_get(2)->regs.a1 == 11 && proc_get(2)->regs.a2 == 22);
		assert(result == (yielding ? proc_reload_result(proc_get(2)) : proc_resume_result(proc_get(1))));
		assert(proc_get(1)->state == (yielding ? PROC_STATE_READY : PROC_STATE_ACQUIRED));
		assert(proc_get(2)->state == (yielding ? PROC_STATE_ACQUIRED : PROC_STATE_READY));
		lock_release();
	}

	for (unsigned replyrecv = 0; replyrecv < 2; ++replyrecv) {
		index_t source = setup_channel(IPC_FLAG_YIELD, &sink);
		lock_acquire();
		status = &test_current->regs.a0;
		assert(ipc_call(1, source, data, CAPTY_NONE, 0, status) == proc_reload_result(proc_get(2)));
		assert(*status == (word_t)ERR_TIMEOUT);
		assert(!(proc_get(1)->state & PROC_STATE_ACQUIRED));
		test_current = proc_get(2);
		status = &test_current->regs.a0;
		proc_result_t result = replyrecv ? ipc_replyrecv(2, sink, data, CAPTY_NONE, 0, 0, status)
					 : ipc_reply(2, sink, data, CAPTY_NONE, 0, status);
		assert(result == proc_reload_result(proc_get(1)));
		assert(*status == ERR_SUCCESS);
		assert(!(proc_get(2)->state & PROC_STATE_ACQUIRED));
		assert(proc_get(1)->state == PROC_STATE_ACQUIRED);
		lock_release();
	}

	index_t source = setup_channel(0, &sink);
	lock_acquire();
	status = &test_current->regs.a0;
	assert(ipc_call(1, source, data, CAPTY_NONE, 0, status) == 0);
	assert(*status == (word_t)ERR_TIMEOUT);
	assert(proc_get(1)->state & PROC_STATE_BLOCKED);
	assert(!(proc_get(1)->state & PROC_STATE_ACQUIRED));
	assert(proc_get(2)->state == PROC_STATE_READY);
	test_current = proc_get(2);
	assert(proc_acquire(2));
	status = &test_current->regs.a0;
	assert(ipc_replyrecv(2, sink, data, CAPTY_NONE, 0, 0, status) == 0);
	assert(*status == ERR_SUCCESS);
	assert(proc_get(1)->state == PROC_STATE_READY);
	assert(proc_get(2)->state & PROC_STATE_BLOCKED);
	assert(!(proc_get(2)->state & PROC_STATE_ACQUIRED));
	lock_release();

	reset();
	lock_acquire();
	int async_sink = ipc_derive(1, 0, 2, 4, IPC_MODE_ASYNC, IPC_FLAG_YIELD);
	assert(async_sink >= 0);
	int async_source = ipc_derive(2, async_sink, 1, 1, IPC_MODE_ASYNC, IPC_FLAG_YIELD);
	assert(async_source >= 0);
	assert(proc_acquire(1));
	proc_resume(2);
	status = &test_current->regs.a0;
	assert(ipc_asend(1, async_source, 42, status) == proc_reload_result(proc_get(2)));
	assert(*status == ERR_SUCCESS);
	assert(proc_get(1)->state == PROC_STATE_READY);
	assert(proc_get(2)->state == PROC_STATE_ACQUIRED);
	word_t message;
	assert(ipc_arecv(2, async_sink, &message) == ERR_SUCCESS);
	assert(message == 42);
	lock_release();
}

static void test_derivation_fuel_invariant(void)
{
	reset();
	mon_t mon;
	assert(mon_derive(1, 0, 2, 0) == ERR_INVALID_ARGUMENT);
	assert(mon_derive(1, 0, 2, MAX_MONITOR_FUEL) == ERR_INVALID_ARGUMENT);
	int mon_child = mon_derive(1, 0, 2, MAX_MONITOR_FUEL - 1);
	assert(mon_child == 1);
	assert(mon_derive(2, mon_child, 3, MAX_MONITOR_FUEL - 1) == ERR_INVALID_ARGUMENT);
	assert(mon_introspect(1, 0, 0, &mon) == ERR_SUCCESS);
	assert(mon.owner == 1 && mon.cfree == 1 && mon.csize == MAX_MONITOR_FUEL);

	ipc_t ipc;
	assert(ipc_derive(1, 0, 2, 0, IPC_MODE_NONE, 0) == ERR_INVALID_ARGUMENT);
	assert(ipc_derive(1, 0, 2, MAX_IPC_FUEL, IPC_MODE_NONE, 0) == ERR_INVALID_ARGUMENT);
	int ipc_child = ipc_derive(1, 0, 2, 1, IPC_MODE_NONE, 0);
	assert(ipc_child == MAX_IPC_FUEL - 1);
	assert(ipc_derive(2, ipc_child, 2, 0, IPC_MODE_ASYNC, 0) == ERR_INVALID_ARGUMENT);
	assert(ipc_derive(2, ipc_child, 2, 1, IPC_MODE_ASYNC, 0) == ERR_INVALID_ARGUMENT);
	assert(ipc_revoke(1, 0) == 0);
	assert(ipc_introspect(1, 0, 0, &ipc) == ERR_SUCCESS);
	assert(ipc.cfree == MAX_IPC_FUEL && ipc.csize == MAX_IPC_FUEL);

	assert(mem_derive(1, 0, 2, 0, MEM_PERM_RW, 0x80000000, 0x1000) == ERR_INVALID_ARGUMENT);
	assert(mem_derive(1, 0, 2, MAX_MEMORY_FUEL, MEM_PERM_RW, 0x80000000, 0x1000) == ERR_INVALID_ARGUMENT);
	assert(tsl_derive(1, 0, 2, 0, true, 1) == ERR_INVALID_ARGUMENT);
	assert(tsl_derive(1, 0, 2, MAX_TIME_FUEL, true, 1) == ERR_INVALID_ARGUMENT);
}

int main(void)
{
	test_scheduler();
	test_ticket_lock();
	test_suspend_release();
	test_lazy_pmp();
	test_revocation();
	test_revocation_progress();
	test_process_result_encoding();
	test_trap_results();
	test_pmp_reload_policy();
	test_monitor_grant();
	test_capability_syscalls();
	test_derive_syscalls();
	test_ipc_handoffs();
	test_individual_handler_results();
	test_operation_handoffs();
	test_shared_ipc_transfers();
	test_shared_pmp_handlers();
	test_derivation_fuel_invariant();
	puts("Multicore regression tests passed.");
	return 0;
}

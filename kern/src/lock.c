#include "lock.h"

#ifdef SMP
static word_t next_ticket;
static word_t serving_ticket;

void lock_init(void)
{
	next_ticket = 0;
	serving_ticket = 0;
}

void lock_acquire(void)
{
	word_t ticket = __atomic_fetch_add(&next_ticket, 1, __ATOMIC_RELAXED);
	while (__atomic_load_n(&serving_ticket, __ATOMIC_ACQUIRE) != ticket) {
		__asm__ volatile("nop");
	}
}

void lock_release(void)
{
	word_t ticket = __atomic_load_n(&serving_ticket, __ATOMIC_RELAXED);
	__atomic_store_n(&serving_ticket, ticket + 1, __ATOMIC_RELEASE);
}

bool lock_contended(void)
{
	return __atomic_load_n(&next_ticket, __ATOMIC_RELAXED)
	       != __atomic_load_n(&serving_ticket, __ATOMIC_RELAXED) + 1;
}
#else

void lock_init(void)
{
}

void lock_acquire(void)
{
}

void lock_release(void)
{
}

bool lock_contended(void)
{
	return false;
}
#endif

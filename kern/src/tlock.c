#include <tlock.h>



void tlock_acquire(tlock_t *l)
{
	word_t my_ticket = __atomic_fetch_add(&(l->next), 1, __ATOMIC_ACQUIRE);

	while (__atomic_load_n(&l->released, __ATOMIC_ACQUIRE) != my_ticket) {
		/* Spin. */
	}
}

void tlock_release(tlock_t *l)
{
	__atomic_store_n(&l->released, l->released + 1, __ATOMIC_RELEASE);
}

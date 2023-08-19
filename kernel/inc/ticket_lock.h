#pragma once
/**
 * @file lock.h
 *
 * This header file contains the declarations for the ticket lock
 * synchronization primitive, which allows multiple threads to acquire
 * a lock in a fair and orderly manner. The ticket lock works by assigning
 * each thread a unique ticket number, and then serving threads in the order
 * of their ticket numbers. This ensures that threads acquire the lock in
 * the order that they request it, preventing starvation and ensuring
 * fairness.
 *
 * @copyright MIT License
 * @author Henrik Karlsson (henrik10@kth.se)
 */
#include <stdint.h>

typedef struct {
	/// next ticket number to be issued.
	unsigned int next_ticket;
	/// ticket number currently being served.
	unsigned int serving_ticket;
} ticket_lock_t;

static inline void tl_init(ticket_lock_t *lock)
{
	lock->next_ticket = 0;
	lock->serving_ticket = 0;
}

/**
 * Acquire a ticket lock.
 *
 * @param lock Pointer to the ticket lock to acquire.
 */
static inline void tl_acq(ticket_lock_t *lock)
{
	// Increment next ticket number and return the previous value
	unsigned long ticket
	    = __atomic_fetch_add(&lock->next_ticket, 1, __ATOMIC_RELAXED);

	// Wait until our ticket number is being served
	while (__atomic_load_n(&lock->serving_ticket, __ATOMIC_ACQUIRE)
	       != ticket) {
		// Wait until our ticket number is being served
	}
}

/**
 * Release a ticket lock.
 *
 * @param lock Pointer to the ticket lock to release.
 */
static inline void tl_rel(ticket_lock_t *lock)
{
	// Atomically increment the serving ticket number
	__atomic_fetch_add(&lock->serving_ticket, 1, __ATOMIC_RELEASE);
}

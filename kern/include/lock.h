#pragma once

#include "types.h"

/**
 * Initializes the lock.
 */
void lock_init(void);

/**
 * Acquires the lock in FIFO order. An issued ticket cannot be cancelled.
 */
void lock_acquire(void);

/**
 * Releases the lock.
 */
void lock_release(void);

/**
 * Checks for queued harts while holding the lock.
 */
bool lock_contended(void);

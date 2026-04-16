#pragma once
#include <types.h>

typedef struct {
	word_t next;
	word_t released;
} tlock_t;

void tlock_acquire(tlock_t *l);
void tlock_release(tlock_t *l);

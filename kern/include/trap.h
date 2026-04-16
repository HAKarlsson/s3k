#pragma once
#include <types.h>
#include <proc.h>


void trap_entry(void);
void trap_resume(proc_t *proc);
void trap_exit(proc_t *proc);
proc_t* trap_handler(proc_t *proc, word_t mcause, word_t mtval);

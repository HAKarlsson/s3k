#pragma once

#include "cap_table.h"
#include "proc.h"

void ipc_clear(uint64_t channel);
int ipc_recv(proc_t *proc, cptr_t cptr);
int ipc_send(proc_t *proc, cptr_t cptr, uint64_t[4], uint64_t yield);
int ipc_sendrecv(proc_t *proc, cptr_t cptr, uint64_t[4]);

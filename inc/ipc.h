#pragma once

#include "cap_table.h"
#include "excpt.h"
#include "proc.h"

void ipc_clear(uint64_t channel);
excpt_t ipc_recv(proc_t *proc, cptr_t cptr);
excpt_t ipc_send(proc_t *proc, cptr_t cptr, uint64_t[4], uint64_t yield);
excpt_t ipc_sendrecv(proc_t *proc, cptr_t cptr, uint64_t[4]);

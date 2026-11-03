#pragma once

#include "types.h"

/* Requires the kernel lock; authorization of the destination is caller-owned. */
int cap_transfer(pid_t owner, capty_t type, index_t i, pid_t target);

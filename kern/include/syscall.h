#pragma once

#include "proc.h"
#include "types.h"

/**
 * System call handler.
 * Returns zero after releasing the caller, a positive PCB address to resume,
 * or its negative value to reload PMP before resuming.
 */
proc_result_t syscall_handler(void);

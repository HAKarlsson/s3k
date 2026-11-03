#pragma once

#include "proc.h"
#include "types.h"

/**
 * @brief Handles hardware interrupts.
 *
 * This function is called when a hardware interrupt occurs.
 * It releases the interrupted process and returns zero to wait for scheduling.
 *
 * @param cause The cause of the interrupt.
 * @param tval The trap value associated with the interrupt.
 * @return Zero after releasing the interrupted process.
 */
proc_result_t interrupt_handler(word_t cause, word_t tval);

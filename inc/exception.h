/**
 * @file exception.h
 * @brief User exception handler
 * @copyright MIT License
 * @author Henrik Karlsson (henrik10@kth.se)
 */

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include "proc.h"

#include <stdint.h>

/**
 * @brief Handle an exception
 *
 * This function handles exceptions by checking the mcause register for
 * specific types of exceptions. If the exception is an illegal instruction
 * and the mtval register contains a valid return instruction, it calls the
 * handle_ret() function. Otherwise, it calls the handle_default() function
 * to handle the exception.
 *
 * @param proc  Pointer to the process that encountered the exception
 * @param mcause  The value of the mcause register
 * @param mepc  The value of the mepc register
 * @param mtval  The value of the mtval register
 * @param Returns proc.
 */
struct proc *handle_exception(struct proc *proc, uint64_t mcause, uint64_t mepc,
			      uint64_t mtval);

#endif /* __EXCEPTION_H__ */

#pragma once
/**
 * @file trap.h
 * @brief Declares trap entry/exit functions.
 * @copyright MIT License
 * @author Henrik Karlsson (henrik10@kth.se)
 */
#include "common.h"

void trap_entry(void) __attribute__((noreturn));
void trap_exit(void) __attribute__((noreturn));

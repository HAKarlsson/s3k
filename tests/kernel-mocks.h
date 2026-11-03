#pragma once

#include "csr.h"
#include "current.h"

extern proc_t *test_current;
extern word_t test_mip;
extern hart_t test_hart;

#define current test_current
#define csrr_mip() test_mip
#define csrr_mhartid() test_hart

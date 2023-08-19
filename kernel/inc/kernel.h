#include <stdint.h>
void kernel_init();
uint64_t get_hartid();
void preempt_enable();
void preempt_disable();
void kernel_lock();
void kernel_unlock();

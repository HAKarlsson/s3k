#ifndef __CURRENT_H__
#define __CURRENT_H__
#include "proc.h"

register proc_t *current __asm__("tp");

#endif /* __CURRENT_H__ */

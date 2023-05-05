#ifndef __PLATFORM_H__
#define __PLATFORM_H__

// Min and max hart ID.
#define MIN_HARTID 0
#define MAX_HARTID 3
// Harts 0,1,2,3
#define HART_COUNT 4

// RTC ticks per second
#define TICKS_PER_SECOND 1000000ull

/// Stack size of 1 KiB
#define LOG_STACK_SIZE 8

#endif /* __PLATFORM_H__ */

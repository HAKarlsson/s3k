#ifndef __PLATFORM_H__
#define __PLATFORM_H__

// Min and max hart ID.
#define MIN_HARTID 1
#define MAX_HARTID 4
// Harts 1,2,3,4
#define NUM_OF_HARTS 4

// RTC ticks per second
#define TICKS_PER_SECOND 1000000ull

#define CLINT 0x2000000ull

/// Stack size of 256 B
#define LOG_STACK_SIZE 8

#endif /* __PLATFORM_H__ */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

// Min and max hart ID.
#define MIN_HARTID 0
#define MAX_HARTID 3

// Harts 0,1,2,3
#define N_HART 4

#define N_PMP 8

// RTC ticks per second
#define TICKS_PER_SECOND 1000000ull

/// Stack size of 256 KiB
#define LOG_STACK_SIZE 8

// Core-local interrupt registers
#define CLINT 0x2000000ull

// Initial capabilities.
#define INIT_CAPS                                                           \
	{                                                                   \
		pmp_cap(0x20005fff, MEM_RWX),                               \
		    memory_cap(0x80020, 0x80, MEM_RWX),                     \
		    memory_cap(0x10000, 0x1, MEM_RW),                       \
		    memory_cap(0x200b, 0x1, MEM_R), time_cap(0, 0, N_SLOT), \
		    time_cap(1, 0, N_SLOT), time_cap(2, 0, N_SLOT),         \
		    time_cap(3, 0, N_SLOT), monitor_cap(0, N_PROC),         \
		    channel_cap(0, N_CHAN)                                  \
	}

#endif /* __PLATFORM_H__ */

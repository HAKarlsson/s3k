// Default kernel configuration
#ifndef __CONFIG_H__
#define __CONFIG_H__

// Number of user processes
#define NUM_OF_PROCESSES 8

// Number of capabilities per process.
#define NUM_OF_CAPABILITIES 32

// Number of IPC channels.
#define NUM_OF_CHANNELS 16

// Number of slices per period
#define NUM_OF_FRAMES 64ull

// Length of time slice in ticks.
#define FRAME_LENGTH (TICKS_PER_SECOND / NUM_OF_FRAMES / 100ull)

// Number of slack ticks per slice
#define SLACK_LENGTH 10000

// If debugging, uncomment
#define NDEBUG

// If instrumenting
#define INSTRUMENTATION

#endif /* __CONFIG_H__ */

// Kernel configuration
#ifndef __CONFIG_H__
#define __CONFIG_H__

// Number of user processes
#define PROC_COUNT 8
// Number of capabilities per process.
#define CAP_COUNT 32
// Number of IPC channels.
#define CHANNEL_COUNT 16
// Number of slices per period
#define SLICE_COUNT 64ull
// Length of time slice in ticks.
#define SLICE_LENGTH (TICKS_PER_SECOND / SLICE_COUNT / 100ull)
// Number of slack ticks per slice
#define SLACK_LENGTH 10000

// If debugging, uncomment
#define NDEBUG

// If instrumenting
#define INSTRUMENTATION

#endif /* __CONFIG_H__ */

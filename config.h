// Kernel configuration
#ifndef __CONFIG_H__
#define __CONFIG_H__

// Number of user processes
#define NPROC 16
// Number of capabilities per process.
#define NCAP 32
// Number of IPC channels.
#define NCHANNEL 16
// Number of slices per period
#define NSLICE 64ull
// Number of ticks per slice
#define NTICK (TICKS_PER_SECOND / NSLICE / 100ull)
// Number of slack ticks per slice
#define NSLACK 10000

// If debugging, uncomment
#define NDEBUG

#endif /* __CONFIG_H__ */

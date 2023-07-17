#pragma once

#include "cap_types.h"
#include "cap_utils.h"

static const cap_t init_caps[] = {
	[0] = CAP_PMP(0x20005fff, CAP_RWX), // Boot memory [0x8001000, 0x8002000]
	[1] = CAP_MEMORY(0x80020, 0x80, CAP_RWX), // Main memory [0x80020000, 0x800a000]
	[2] = CAP_MEMORY(0x10000, 0x1, CAP_RW), // UART [0x10000000, 0x10001000]
	[3] = CAP_MEMORY(0x2000b, 0x1, CAP_R), // TIME [0x2000b000, 0x2000c000]
	[4] = CAP_TIME(0, 0, NUM_OF_FRAMES),
	[5] = CAP_TIME(1, 0, NUM_OF_FRAMES),
	[6] = CAP_TIME(2, 0, NUM_OF_FRAMES),
	[7] = CAP_TIME(3, 0, NUM_OF_FRAMES),
	[8] = CAP_MONITOR(0, NUM_OF_PROCESSES),
	[9] = CAP_CHANNEL(0, NUM_OF_CHANNELS)
};

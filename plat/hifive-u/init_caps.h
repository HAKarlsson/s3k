#pragma once

#include "cap_types.h"

static const cap_t init_caps[] = {
	[0] = CAP_PMP(0x20005fff, CAP_RWX),
	[1] = CAP_MEMORY(0x0020, 0x8000, 0x10, CAP_RWX),
	[2] = CAP_MEMORY(0x0010, 0x0011, 0x2, CAP_RW),
	[3] = CAP_MEMORY(0x200b, 0x200c, 0x0, CAP_R),
	[4] = CAP_TIME(1, 0, NUM_OF_FRAMES),
	[5] = CAP_TIME(2, 0, NUM_OF_FRAMES),
	[6] = CAP_TIME(3, 0, NUM_OF_FRAMES),
	[7] = CAP_TIME(4, 0, NUM_OF_FRAMES),
	[8] = CAP_MONITOR(0, NUM_OF_PROCESSES),
	[9] = CAP_CHANNEL(0, NUM_OF_CHANNELS)
};

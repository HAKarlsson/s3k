#pragma once

#include "cap_types.h"
#include "cap_utils.h"

static const cap_t init_caps[] = {
	[0] = CAP_PMP(0x20005fff, CAP_RWX),
	[1] = CAP_MEMORY(0x80010, 0x10, CAP_RWX),
	[2] = CAP_MEMORY(0x10000, 0x1, CAP_RW),
	[3] = CAP_MEMORY(0x2000b, 0x1, CAP_R),
	[4] = CAP_TIME(1, 0, NUM_OF_FRAMES),
	[5] = CAP_TIME(2, 0, NUM_OF_FRAMES),
	[6] = CAP_TIME(3, 0, NUM_OF_FRAMES),
	[7] = CAP_TIME(4, 0, NUM_OF_FRAMES),
	[8] = CAP_MONITOR(0, NUM_OF_PROCESSES),
	[9] = CAP_CHANNEL(0, NUM_OF_CHANNELS)
};

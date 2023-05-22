#pragma once

#include "cap_types.h"

#include <stdbool.h>

bool cap_can_revoke(cap_t, cap_t);
bool cap_can_derive(cap_t, cap_t);

#define CAP_NULL         \
	(cap_t)          \
	{                \
		.raw = 0 \
	}

#pragma once
#if PLATFORM == virt
#include "../plat/virt.h"
#elif PLATFORM == hifive_u
#include "../plat/hifive_u.h"
#else
#error "Unknown platform"
#endif

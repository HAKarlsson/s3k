#include "cap_utils.h"

#include "pmp.h"

// Check if bits in a subsets b
static bool subset(uint64_t a, uint64_t b)
{
	return (a & b) == a;
}

bool cap_can_revoke(cap_t a, cap_t b)
{
	switch (a.type) {
	case CAPTY_TIME: {
		if (b.type == CAPTY_TIME) {
			uint64_t a_hartid = a.time.hartid;
			uint64_t a_begin = a.time.base;
			uint64_t a_end = a.time.base + a.time.len;
			uint64_t b_hartid = b.time.hartid;
			uint64_t b_begin = b.time.base;
			uint64_t b_end = b.time.base + b.time.len;
			return a_hartid == b_hartid
			       && a_begin <= b_begin && b_end <= a_end;
		}
	} break;

	case CAPTY_MEMORY: {
		if (b.type == CAPTY_MEMORY) {
			uint64_t a_begin = a.memory.base;
			uint64_t a_end = a.memory.base + a.memory.len;
			uint64_t b_begin = b.memory.base;
			uint64_t b_end = b.memory.base + b.memory.len;
			return a_begin <= b_begin && b_end <= a_end;
		}
		if (b.type == CAPTY_PMP) {
			uint64_t a_begin = (uint64_t)a.memory.base << 12;
			uint64_t a_end = (uint64_t)(a.memory.base + a.memory.len) << 12;
			uint64_t b_begin, b_end;
			pmp_napot_decode(b.pmp.addr, &b_begin, &b_end);
			return a_begin <= b_begin && b_end <= a_end;
		}
	} break;

	case CAPTY_MONITOR: {
		if (b.type == CAPTY_MONITOR) {
			uint64_t a_begin = a.monitor.base;
			uint64_t a_end = a.monitor.base + a.monitor.len;
			uint64_t b_begin = b.monitor.base;
			uint64_t b_end = b.monitor.base + b.monitor.len;
			return a_begin <= b_begin && b_end <= a_end;
		}
	} break;

	case CAPTY_CHANNEL: {
		if (b.type == CAPTY_CHANNEL) {
			uint64_t a_begin = a.channel.base;
			uint64_t a_end = a.channel.base + a.channel.len;
			uint64_t b_begin = b.channel.base;
			uint64_t b_end = b.channel.base + b.channel.len;
			return a_begin <= b_begin && b_end <= a_end;
		}
		if (b.type == CAPTY_SOCKET) {
			uint64_t a_begin = a.channel.base;
			uint64_t a_end = a.channel.base + a.channel.len;
			uint64_t b_channel = b.socket.channel;
			return a_begin <= b_channel && b_channel < a_end;
		}
	} break;

	case CAPTY_SOCKET: {
		if (b.type == CAPTY_SOCKET) {
			uint64_t a_channel = a.socket.channel;
			uint64_t b_channel = b.socket.channel;
			return a_channel == b_channel;
		}
	} break;
	}

	return false;
}

bool cap_can_derive(cap_t a, cap_t b)
{
	switch (a.type) {
	case CAPTY_TIME: {
		if (b.type == CAPTY_TIME) {
			uint64_t a_hartid = a.time.hartid;
			uint64_t a_free = a.time.base + a.time.alloc;
			uint64_t a_end = a.time.base + a.time.len;
			uint64_t b_hartid = b.time.hartid;
			uint64_t b_begin = b.time.base;
			uint64_t b_alloc = b.time.alloc;
			uint64_t b_end = b.time.base + b.time.len;
			return a_hartid == b_hartid && b_alloc == 0
			       && a_free == b_begin && b_end <= a_end;
		}
	} break;

	case CAPTY_MEMORY: {
		if (b.type == CAPTY_MEMORY) {
			uint64_t a_free = a.memory.base + a.memory.alloc;
			uint64_t a_end = a.memory.base + a.memory.len;
			uint64_t a_lock = b.memory.lock;
			uint64_t a_rwx = a.memory.rwx;
			uint64_t b_begin = b.memory.base;
			uint64_t b_alloc = b.memory.alloc;
			uint64_t b_end = b.memory.base + b.memory.len;
			uint64_t b_lock = b.memory.lock;
			uint64_t b_rwx = b.memory.rwx;
			return a_free == b_begin && b_end <= a_end
			       && b_alloc == 0 && !b_lock && !a_lock
			       && subset(b_rwx, a_rwx);
		}
		if (b.type == CAPTY_PMP) {
			uint64_t a_free = (uint64_t)(a.memory.base + a.memory.alloc) << 12;
			uint64_t a_end = (uint64_t)(a.memory.base + a.memory.len) << 12;
			uint64_t a_rwx = a.memory.rwx;

			uint64_t b_base, b_size;
			pmp_napot_decode(b.pmp.addr, &b_base, &b_size);
			uint64_t b_rwx = b.pmp.rwx;

			return a_free <= b_base && (b_base + b_size) <= a_end
			       && subset(b_rwx, a_rwx);
		}
	} break;

	case CAPTY_MONITOR: {
		if (b.type == CAPTY_MONITOR) {
			uint64_t a_free = a.monitor.base + a.monitor.alloc;
			uint64_t a_end = a.monitor.base + a.monitor.len;
			uint64_t b_begin = b.monitor.base;
			uint64_t b_alloc = b.monitor.alloc;
			uint64_t b_end = b.monitor.base + b.monitor.len;
			return a_free == b_begin && b_end <= a_end
			       && b_alloc == 0;
		}
	} break;

	case CAPTY_CHANNEL: {
		if (b.type == CAPTY_CHANNEL) {
			uint64_t a_free = a.channel.base + a.channel.alloc;
			uint64_t a_end = a.channel.base + a.channel.len;
			uint64_t b_begin = b.channel.base;
			uint64_t b_alloc = b.channel.alloc;
			uint64_t b_end = b.channel.base + b.channel.len;
			return a_free == b_begin && b_end <= a_end
			       && b_alloc == 0;
		}
		if (b.type == CAPTY_SOCKET) {
			uint64_t a_free = a.channel.base + a.channel.alloc;
			uint64_t a_end = a.channel.base + a.channel.len;
			uint64_t b_channel = b.socket.channel;
			uint64_t b_tag = b.socket.tag;
			return a_free == b_channel && b_channel < a_end
			       && b_tag == 0;
		}
	} break;

	case CAPTY_SOCKET: {
		if (b.type == CAPTY_SOCKET) {
			uint64_t a_channel = a.socket.channel;
			uint64_t a_tag = a.socket.tag;
			uint64_t b_channel = b.socket.channel;
			uint64_t b_tag = b.socket.tag;
			return a_channel == b_channel
			       && a_tag == 0 && b_tag != 0;
		}
	} break;
	}

	return false;
}

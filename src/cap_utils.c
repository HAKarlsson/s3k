#include "cap_utils.h"

#include "pmp.h"

static bool subset(uint64_t a, uint64_t b)
{
	return (a & b) == a;
}

bool cap_can_revoke(cap_t a, cap_t b)
{
	switch (a.type) {
	case CAPTY_TIME: {
		if (b.type == CAPTY_TIME) {
			return a.time.hartid == b.time.hartid
			       && a.time.begin <= b.time.begin
			       && b.time.end <= a.time.end;
		}
	} break;

	case CAPTY_MEMORY: {
		if (b.type == CAPTY_MEMORY) {
			return a.memory.offset == b.memory.offset
			       && a.memory.begin <= b.memory.begin
			       && b.memory.end <= a.memory.end;
		}
		if (b.type == CAPTY_PMP) {
			uint64_t a_begin = (a.memory.begin << 12)
					   + (a.memory.offset << 27);
			uint64_t a_end = (a.memory.end << 12)
					 + (a.memory.offset << 27);
			uint64_t b_begin = pmp_napot_begin(b.pmp.addr);
			uint64_t b_end = pmp_napot_end(b.pmp.addr);
			return a_begin <= b_begin && b_end <= a_end;
		}
	} break;

	case CAPTY_MONITOR: {
		if (b.type == CAPTY_MONITOR) {
			return a.monitor.begin <= b.monitor.begin
			       && b.monitor.end <= a.monitor.end;
		}
	} break;

	case CAPTY_CHANNEL: {
		if (b.type == CAPTY_CHANNEL) {
			return a.channel.begin <= b.channel.begin
			       && b.channel.end <= a.channel.end;
		}
		if (b.type == CAPTY_SOCKET) {
			return a.channel.begin <= b.socket.channel
			       && b.socket.channel < a.channel.end;
		}
	} break;

	case CAPTY_SOCKET: {
		if (b.type == CAPTY_SOCKET) {
			return a.socket.channel == b.socket.channel;
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
			return a.time.hartid == b.time.hartid
			       && b.time.free == b.time.begin
			       && b.time.begin < b.time.end
			       && a.time.free == b.time.begin
			       && b.time.end <= a.time.end;
		}
	} break;

	case CAPTY_MEMORY: {
		if (b.type == CAPTY_MEMORY) {
			return b.memory.free == b.memory.begin
			       && b.memory.begin < b.memory.end
			       && a.memory.offset == b.memory.offset
			       && a.memory.free <= b.memory.begin
			       && b.memory.end <= a.memory.end
			       && a.memory.lock == 0
			       && b.memory.lock == 0
			       && subset(b.memory.rwx, a.memory.rwx);
		}
		if (b.type == CAPTY_PMP) {
			uint64_t a_free = (a.memory.free << 12)
					  + (a.memory.offset << 27);
			uint64_t a_end = (a.memory.end << 12)
					 + (a.memory.offset << 27);
			uint64_t b_begin = pmp_napot_begin(b.pmp.addr);
			uint64_t b_end = pmp_napot_end(b.pmp.addr);
			return a_free <= b_begin && b_end <= a_end
			       && subset(b.pmp.rwx, a.memory.rwx);
		}
	} break;

	case CAPTY_MONITOR: {
		if (b.type == CAPTY_MONITOR) {
			return b.monitor.free == b.monitor.begin
			       && b.monitor.begin < b.monitor.end
			       && a.monitor.free <= b.monitor.begin
			       && b.monitor.end <= a.monitor.end;
		}
	} break;

	case CAPTY_CHANNEL: {
		if (b.type == CAPTY_CHANNEL) {
			return b.channel.free == b.channel.begin
			       && b.channel.begin < b.channel.end
			       && a.channel.free <= b.channel.begin
			       && b.channel.end <= a.channel.end;
		}
		if (b.type == CAPTY_SOCKET) {
			return a.channel.free <= b.socket.channel
			       && b.socket.channel < a.channel.end
			       && b.socket.tag == 0;
		}
	} break;

	case CAPTY_SOCKET: {
		if (b.type == CAPTY_SOCKET) {
			return a.socket.channel == b.socket.channel
			       && a.socket.tag == 0 && b.socket.tag > 0;
		}
	} break;
	}

	return false;
}

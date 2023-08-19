#include "cap/types.h"

#include "pmp.h"

/** Check if all bits in `a` are in `b`. */
static inline bool is_bit_subset(uint64_t a, uint64_t b)
{
	return (a & b) == a;
}

static bool is_range_subset(uint64_t a_begin, uint64_t a_end, uint64_t b_begin,
			    uint64_t b_end)
{
	return a_begin <= b_begin && b_end <= a_end;
}

static bool is_range_prefix(uint64_t a_begin, uint64_t a_end, uint64_t b_begin,
			    uint64_t b_end)
{
	return a_begin == b_begin && b_end <= a_end;
}

cap_t time_cap(uint64_t hart, uint64_t begin, uint64_t end)
{
	cap_t cap;
	cap.type = CAPTY_TIME;
	cap.time.begin = begin & 0xFFFF;
	cap.time.mark = cap.time.begin;
	cap.time.end = end & 0xFFFF;
	return cap;
}

cap_t memory_cap(uint64_t begin, uint64_t end, uint64_t rwx)
{
	uint64_t offset = begin >> 15;

	cap_t cap;
	cap.type = CAPTY_MEMORY;
	cap.memory.offset = offset & 0xFF;
	cap.memory.begin = (begin - (offset << 15)) & 0xFFFF;
	cap.memory.end = (end - (offset << 15)) & 0xFFFF;
	cap.memory.mark = cap.memory.begin;
	cap.memory.rwx = (rwx & 0x7);
	cap.memory.lock = 0;
	return cap;
}

cap_t pmp_cap(uint64_t addr, uint64_t rwx)
{
	cap_t cap;
	cap.type = CAPTY_PMP;
	cap.pmp.addr = addr & 0xFFFFFFFFFF;
	cap.pmp.rwx = rwx & 0x7;
	cap.pmp.used = 0;
	cap.pmp.index = 0;
	return cap;
}

cap_t monitor_cap(uint64_t begin, uint64_t end)
{
	cap_t cap;
	cap.type = CAPTY_MONITOR;
	cap.monitor.begin = begin & 0xFFFF;
	cap.monitor.end = end & 0xFFFF;
	cap.monitor.mark = cap.monitor.begin;
	return cap;
}

cap_t channel_cap(uint64_t begin, uint64_t end)
{
	cap_t cap;
	cap.type = CAPTY_CHANNEL;
	cap.channel.begin = begin & 0xFFFF;
	cap.channel.end = end & 0xFFFF;
	cap.channel.mark = cap.channel.begin;
	return cap;
}

cap_t server_cap(uint64_t channel, uint64_t mode, uint64_t perm, uint64_t tag)
{
	cap_t cap;
	cap.type = CAPTY_SERVER;
	cap.server.channel = channel & 0xFFFF;
	cap.server.mode = mode & 0xF;
	cap.server.perm = perm & 0xFF;
	return cap;
}

cap_t client_cap(uint64_t channel, uint64_t mode, uint64_t perm, uint64_t tag)
{
	cap_t cap;
	cap.type = CAPTY_CLIENT;
	cap.client.channel = channel & 0xFFFF;
	cap.client.mode = mode & 0xF;
	cap.client.perm = perm & 0xFF;
	cap.client.tag = tag & 0xFFFFFFFF;
	return cap;
}

bool cap_can_revoke(cap_t parent, cap_t child)
{
	if (parent.type != child.type) {
		if (parent.type == CAPTY_MEMORY && child.type == CAPTY_PMP) {
			uint64_t parent_offset = parent.memory.offset << 27;
			uint64_t parent_begin = parent.memory.begin << 12;
			uint64_t parent_end = parent.memory.end << 12;
			uint64_t child_begin, child_end;
			pmp_napot_decode(child.pmp.addr, &child_begin,
					 &child_end);
			return is_range_subset(parent_offset + parent_begin,
					       parent_offset + parent_end,
					       child_begin, child_end);
		} else if (parent.type == CAPTY_CHANNEL
			   && child.type == CAPTY_SERVER) {
			return is_range_subset(parent.channel.begin,
					       parent.channel.end,
					       child.server.channel,
					       child.server.channel + 1);
		} else if (parent.type == CAPTY_CHANNEL
			   && child.type == CAPTY_CLIENT) {
			return is_range_subset(parent.channel.begin,
					       parent.channel.end,
					       child.client.channel,
					       child.client.channel + 1);
		} else if (parent.type == CAPTY_SERVER
			   && child.type == CAPTY_CLIENT) {
			return parent.server.channel == child.server.channel;
		}
		return false;
	}

	switch (parent.type) {
	case CAPTY_TIME:
		return parent.time.hart == child.time.hart
		       && is_range_subset(parent.time.begin, parent.time.end,
					  child.time.begin, child.time.end);
	case CAPTY_MEMORY:
		return is_range_subset(parent.memory.begin, parent.memory.end,
				       child.memory.begin, child.memory.end);
	case CAPTY_MONITOR:
		return is_range_subset(parent.monitor.begin, parent.monitor.end,
				       child.monitor.begin, child.monitor.end);
	case CAPTY_CHANNEL:
		return is_range_subset(parent.channel.begin, parent.channel.end,
				       child.channel.begin, child.channel.end);
	default:
		return false;
	}
}

bool cap_can_derive(cap_t parent, cap_t child)
{
	if (parent.type != child.type) {
		if (parent.type == CAPTY_MEMORY && child.type == CAPTY_PMP) {
			uint64_t parent_offset = parent.memory.offset << 27;
			uint64_t parent_mark = (uint64_t)parent.memory.mark
					       << 12;
			uint64_t parent_end = (uint64_t)parent.memory.end << 12;
			uint64_t child_begin, child_end;
			pmp_napot_decode(child.pmp.addr, &child_begin,
					 &child_end);
			return is_range_subset(parent_offset + parent_mark,
					       parent_offset + parent_end,
					       child_begin, child_end)
			       && is_bit_subset(child.pmp.rwx, parent.pmp.rwx)
			       && !child.pmp.used;
		} else if (parent.type == CAPTY_CHANNEL
			   && child.type == CAPTY_SERVER) {
			return is_range_subset(parent.channel.mark,
					       parent.channel.end,
					       child.server.channel,
					       child.server.channel + 1);
		} else if (parent.type == CAPTY_SERVER
			   && child.type == CAPTY_CLIENT) {
			return parent.server.channel == child.client.channel
			       && parent.server.mode == child.client.mode
			       && parent.server.perm == child.client.perm;
		}
		return false;
	}

	switch (parent.type) {
	case CAPTY_TIME:
		return parent.time.hart == child.time.hart
		       && is_range_prefix(parent.time.mark, parent.time.end,
					  child.time.begin, child.time.end)
		       && child.time.begin == child.time.mark
		       && child.time.begin < child.time.end;
	case CAPTY_MEMORY:
		return is_range_prefix(parent.memory.mark, parent.memory.end,
				       child.memory.begin, child.memory.end)
		       && child.memory.begin == child.memory.mark
		       && child.memory.begin < child.memory.end
		       && child.memory.offset == parent.memory.offset
		       && is_bit_subset(child.pmp.rwx, parent.pmp.rwx);
	case CAPTY_MONITOR:
		return is_range_prefix(parent.monitor.mark, parent.monitor.end,
				       child.monitor.begin, child.monitor.end)
		       && child.monitor.begin == child.monitor.mark
		       && child.monitor.begin < child.monitor.end;
	case CAPTY_CHANNEL:
		return is_range_prefix(parent.channel.mark, parent.channel.end,
				       child.channel.begin, child.channel.end)
		       && child.monitor.begin == child.monitor.mark
		       && child.monitor.begin < child.monitor.end;
	default:
		return false;
	}
}

#include "ipc.h"

#include "cap_table.h"

typedef struct {
	proc_t *master;
	proc_t *slave;
} channel_info_t;

static channel_info_t ch_infos[NUM_OF_CHANNELS];

bool do_send(proc_t *sender, proc_t *receiver, uint64_t channel, uint64_t msgs[4])
{
	if (receiver && !proc_ipc_acquire(receiver, channel))
		return false;
	receiver->regs[REG_A0] = EXCPT_NONE;
	receiver->regs[REG_A1] = msgs[0];
	receiver->regs[REG_A2] = msgs[1];
	receiver->regs[REG_A3] = msgs[2];
	receiver->regs[REG_A4] = msgs[3];
	proc_release(receiver);
	return true;
}

excpt_t ipc_recv(proc_t *proc, cptr_t cptr)
{
	cap_t cap = ctable_get_cap(cptr);

	if (cap.type != CAPTY_SOCKET && cap.socket.tag != 0)
		return EXCPT_INVALID_CAP;
	uint64_t channel = cap.socket.channel;

	ch_infos[channel].master = proc;
	proc_ipc_wait(proc, channel);
	return EXCPT_PREEMPT;
	// For send socket: Wait if client.
	// For recv socket: Become receiver and wait.
}

excpt_t ipc_send(proc_t *proc, cptr_t cptr, uint64_t msgs[4], uint64_t yield)
{
	cap_t cap = ctable_get_cap(cptr);

	if (cap.type != CAPTY_SOCKET)
		return EXCPT_INVALID_CAP;

	uint64_t channel = cap.socket.channel;
	channel_info_t ch_info = ch_infos[channel];
	uint64_t is_master = (cap.socket.tag == 0);
	proc_t **receiver = is_master ? &ch_info.slave : &ch_info.master;

	if (do_send(proc, *receiver, channel, msgs)) {
		*receiver = NULL;
		return EXCPT_NONE;
	}

	return EXCPT_EMPTY;
	// For send socket: Send message, if success, become client
	// For recv socket: Send message.
}

excpt_t ipc_sendrecv(proc_t *proc, cptr_t cptr, uint64_t msgs[4])
{
	cap_t cap = ctable_get_cap(cptr);

	if (cap.type != CAPTY_SOCKET && cap.socket.tag != 0)
		return EXCPT_INVALID_CAP;

	uint64_t channel = cap.socket.channel;
	channel_info_t ch_info = ch_infos[channel];
	uint64_t is_master = (cap.socket.tag == 0);
	proc_t **receiver = is_master ? &ch_info.slave : &ch_info.master;

	if (do_send(proc, *receiver, channel, msgs)) {
		*receiver = NULL;
	} else if (!is_master) {
		return -1;
	}

	ch_infos[channel].master = proc;
	proc_ipc_wait(proc, channel);

	return EXCPT_NONE;
	// For send socket: Send to receiver, if success, become client and wait.
	// For recv socket: Send to client, then become receiver unconditionally and wait.
}

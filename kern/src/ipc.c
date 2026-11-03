#include "ipc.h"

#include "cap.h"
#include "current.h"
#include "macro.h"
#include "mem.h"
#include "mon.h"
#include "preempt.h"
#include "rtc.h"
#include "tsl.h"

/**
 * Table of IPC capabilities.
 */
static ipc_t ipc_table[IPC_TABLE_SIZE];

/**
 * Initialize the IPC capabilities.
 */
void ipc_init(void)
{
	ipc_table[0] = (ipc_t){
		.owner = 1,
		.cfree = MAX_IPC_FUEL,
		.csize = MAX_IPC_FUEL,
	};
}

/**
 * Check if the IPC capability is valid for the given owner and index.
 */
bool ipc_valid_access(pid_t owner, index_t i)
{
	return i < ARRAY_SIZE(ipc_table) && ipc_table[i].owner == owner;
}

/**
 * Check if the IPC capability can be derived.
 */
static bool _valid_derivation(ipc_t *cap, fuel_t csize, ipc_mode_t mode, ipc_flag_t flag)
{
	// Check if sufficient cfree is available.
	if (csize == 0 || csize >= cap->cfree) {
		return false;
	}

	if ((cap->mode & IPC_MODE_REVOKE) != 0) {
		// Cannot derive if revoke flag is set.
		return false;
	}

	if (cap->mode == IPC_MODE_NONE) {
		// If deriving a sink IPC capability or NULL IPC capability.
		return mode != IPC_MODE_NONE || flag == 0;
	}
	// If deriving a source IPC capability
	return (cap->mode == mode && cap->flag == flag && csize == 1);
}

/**
 * Check if flags and access rights permit sending a specific capability.
 */
static bool _valid_capability_send(pid_t owner, index_t i, capty_t capty, ipc_flag_t flag)
{
	switch (capty) {
	case CAPTY_NONE:
		return true;
	case CAPTY_MEM:
		return (flag & IPC_FLAG_MEM) && mem_valid_access(owner, i);
	case CAPTY_TSL:
		return (flag & IPC_FLAG_TSL) && tsl_valid_access(owner, i);
	case CAPTY_MON:
		return (flag & IPC_FLAG_MON) && mon_valid_access(owner, i);
	case CAPTY_IPC:
		return (flag & IPC_FLAG_IPC) && ipc_valid_access(owner, i);
	default:
		return false;
	}
}

/**
 * Transfer an IPC capability from one process to another.
 */
int ipc_transfer(pid_t owner, index_t i, pid_t new_owner)
{
	if (UNLIKELY(!ipc_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}
	ipc_table[i].owner = new_owner;
	return ERR_SUCCESS;
}

int ipc_introspect(pid_t owner, index_t i, fuel_t offset, ipc_t *cap)
{
	if (UNLIKELY(!ipc_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	if (offset >= ipc_table[i].csize) {
		return ERR_INVALID_ARGUMENT;
	}

	*cap = ipc_table[i + offset];
	return ERR_SUCCESS;
}

/**
 * Derive a new IPC capability.
 */
int ipc_derive(pid_t owner, index_t i, pid_t target, fuel_t csize, ipc_mode_t mode, ipc_flag_t flag)
{
	if (UNLIKELY(!ipc_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Apply masks to mode and flag.
	mode &= IPC_MODE_MASK;
	flag &= IPC_FLAG_MASK;

	// Check if the derivation is valid.
	if (UNLIKELY(!_valid_derivation(&ipc_table[i], csize, mode, flag))) {
		return ERR_INVALID_ARGUMENT;
	}

	// Subtract delegated capability size from cfree.
	ipc_table[i].cfree -= csize;

	// Calculate the new index for the derived capability.
	index_t j = i + ipc_table[i].cfree;

	// Add the new IPC capability to the table.
	ipc_table[j] = (ipc_t){
		.owner = target,
		.cfree = csize,
		.csize = csize,
		.mode = mode,
		.flag = flag,
		.sink = (ipc_table[i].mode == IPC_MODE_NONE) ? j : ipc_table[i].sink,
		.source = j,
		.opt = 0,
	};

	// Return the index of the new capability.
	return j;
}

/**
 * Revoke an IPC capability.
 */
int ipc_revoke(pid_t owner, index_t i)
{
	if (UNLIKELY(!ipc_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Only non-endpoint IPC capabilities can revoke children.
	if ((ipc_table[i].mode & IPC_MODE_MASK) != IPC_MODE_NONE) {
		return ERR_INVALID_ARGUMENT;
	}

	// Revoke children of IPC capability.
	while (ipc_table[i].cfree < ipc_table[i].csize) {
		// Get the next child index.
		index_t j = i + ipc_table[i].cfree;

		// Invalidate the child capability.
		ipc_table[j].owner = INVALID_PID;

		// Reclaim the child's capability table.
		ipc_table[i].cfree += ipc_table[j].cfree;

		// Set revoke flag to prevent further derivations.
		ipc_table[i].mode = IPC_MODE_REVOKE;

		// Check for preemption.
		if (UNLIKELY(revoke_preempt()))
			break;
	}

	if (ipc_table[i].cfree == ipc_table[i].csize) {
		// Unset revoke flag if all children are revoked.
		ipc_table[i].mode = IPC_MODE_NONE;
	}

	// Return number of unrevoked capability slots.
	return ipc_table[i].csize - ipc_table[i].cfree;
}

/**
 * Delete an IPC capability.
 */
int ipc_delete(pid_t owner, index_t i)
{
	if (UNLIKELY(!ipc_valid_access(owner, i))) {
		return ERR_INVALID_ACCESS;
	}

	// Invalidate the capability.
	ipc_table[i].owner = INVALID_PID;

	return ERR_SUCCESS;
}

/**
 * Send data and potentially a capability to the receiver.
 * For synchronous IPC only!
 */
static void do_send(pid_t receiver, word_t data[2], pid_t owner, capty_t capty, index_t i)
{
	// Send the data to the target process.
	proc_t *proc = proc_get(receiver);
	// Receiver has successfully received the data.
	proc->regs.a0 = ERR_SUCCESS;
	// Copy data.
	proc->regs.a1 = data[0];
	proc->regs.a2 = data[1];
	// Copy capability information.
	proc->regs.a3 = capty;
	proc->regs.a4 = (capty == CAPTY_NONE) ? 0 : i;
	if (capty != CAPTY_NONE) {
		cap_transfer(owner, capty, i, receiver);
	}
}

/**
 * Check if the IPC invocation has valid access.
 */
static bool _ipc_invoke_valid_access(pid_t owner, index_t i, ipc_mode_t mode, bool sink)
{
	if (UNLIKELY(!ipc_valid_access(owner, i))) {
		return false;
	}
	// Check if the IPC capability is in the correct mode.
	if (ipc_table[i].mode != mode) {
		return false;
	}
	// If sink, check if it is a sink capability.
	// Otherwise, check if it is a source capability.
	return sink ? (ipc_table[i].sink == i) : (ipc_table[i].sink != i);
}

static proc_result_t ipc_deliver(pid_t owner, proc_t *receiver, bool yield, bool block, word_t *status, int result)
{
	*status = result;
	if (yield) {
		receiver->timeout = current->timeout;
		proc_release(owner);
		return proc_reload_result(receiver);
	}
	receiver->timeout = 0;
	proc_release(receiver->pid);
	if (block) {
		current->timeout = UINT64_MAX;
		proc_release(owner);
		return 0;
	}
	return proc_resume_result(current);
}

static int ipc_validate_message(pid_t owner, index_t i, capty_t capty, index_t j, ipc_mode_t mode, bool sink)
{
	if (!_ipc_invoke_valid_access(owner, i, mode, sink)) {
		return ERR_INVALID_ACCESS;
	}
	if (!_valid_capability_send(owner, j, capty, ipc_table[i].flag)) {
		return ERR_INVALID_ARGUMENT;
	}
	return ERR_SUCCESS;
}

static proc_result_t ipc_request(pid_t owner, index_t i, word_t data[2], capty_t capty, index_t j,
				word_t *status, ipc_mode_t mode)
{
	*status = ipc_validate_message(owner, i, capty, j, mode, false);
	if (*status != ERR_SUCCESS) {
		return proc_resume_result(current);
	}
	index_t sink = ipc_table[i].sink;
	pid_t receiver = ipc_table[sink].owner;
	if (receiver == INVALID_PID) {
		*status = ERR_INVALID_STATE;
		return proc_resume_result(current);
	}
	bool call = mode == IPC_MODE_BSYNC;
	ipc_flag_t flags = ipc_table[call ? sink : i].flag;
	if ((flags & IPC_FLAG_YIELD)
	    && rtc_get_time() + (uint64_t)ipc_table[sink].opt * TICKS_PER_US >= current->timeout) {
		*status = ERR_INVALID_STATE;
		return proc_resume_result(current);
	}
	if (!proc_ipc_acquire(receiver, sink)) {
		*status = ERR_INVALID_STATE;
		return proc_resume_result(current);
	}
	do_send(receiver, data, owner, capty, j);
	ipc_table[sink].source = i;
	if (call) {
		ipc_table[sink].opt = 0;
		proc_ipc_block(owner, i);
	}
	return ipc_deliver(owner, proc_get(receiver), ipc_table[i].flag & IPC_FLAG_YIELD,
			   call, status, call ? ERR_TIMEOUT : ERR_SUCCESS);
}

proc_result_t ipc_send(pid_t owner, index_t i, word_t data[2], capty_t capty, index_t j, word_t *status)
{
	return ipc_request(owner, i, data, capty, j, status, IPC_MODE_USYNC);
}

/**
 * Receive data and potentially a capability from the sender.
 * For synchronous IPC only!
 */
proc_result_t ipc_recv(pid_t owner, index_t i, uint32_t servtime, word_t *status)
{
	if (!_ipc_invoke_valid_access(owner, i, IPC_MODE_USYNC, true)
	    && !_ipc_invoke_valid_access(owner, i, IPC_MODE_BSYNC, true)) {
		*status = ERR_INVALID_ACCESS;
		return proc_resume_result(current);
	}
	// Go to a receiver state.
	proc_ipc_block(owner, i);
	ipc_table[i].source = i;
	ipc_table[i].opt = servtime;

	current->timeout = UINT64_MAX;
	*status = ERR_SUCCESS;
	proc_release(owner);
	return 0;
}

proc_result_t ipc_call(pid_t owner, index_t i, word_t data[2], capty_t capty, index_t j, word_t *status)
{
	return ipc_request(owner, i, data, capty, j, status, IPC_MODE_BSYNC);
}

static proc_result_t ipc_respond(pid_t owner, index_t i, word_t data[2], capty_t capty, index_t j,
				word_t *status, bool receive, uint32_t servtime)
{
	*status = ipc_validate_message(owner, i, capty, j, IPC_MODE_BSYNC, true);
	if (*status != ERR_SUCCESS) {
		return proc_resume_result(current);
	}
	index_t source = ipc_table[i].source;
	pid_t receiver = ipc_table[source].owner;
	proc_t *next = NULL;
	if (source != i && receiver != INVALID_PID && proc_ipc_acquire(receiver, source)) {
		do_send(receiver, data, owner, capty, j);
		ipc_table[i].source = i;
		next = proc_get(receiver);
	} else if (!receive) {
		*status = ERR_INVALID_STATE;
		return proc_resume_result(current);
	}

	if (!receive) {
		return ipc_deliver(owner, next, ipc_table[i].flag & IPC_FLAG_YIELD, false, status, ERR_SUCCESS);
	}
	// Donate the old deadline before blocking the server indefinitely.
	if (next != NULL) {
		if (ipc_table[i].flag & IPC_FLAG_YIELD) {
			next->timeout = current->timeout;
		} else {
			next->timeout = 0;
			proc_release(receiver);
			next = NULL;
		}
	}
	proc_ipc_block(owner, i);
	ipc_table[i].opt = servtime;
	current->timeout = UINT64_MAX;
	*status = ERR_SUCCESS;
	proc_release(owner);
	return next == NULL ? 0 : proc_reload_result(next);
}

proc_result_t ipc_reply(pid_t owner, index_t i, word_t data[2], capty_t capty, index_t j, word_t *status)
{
	return ipc_respond(owner, i, data, capty, j, status, false, 0);
}

proc_result_t ipc_replyrecv(pid_t owner, index_t i, word_t data[2], capty_t capty, index_t j, uint32_t servtime, word_t *status)
{
	return ipc_respond(owner, i, data, capty, j, status, true, servtime);
}

/**
 * Asynchronously send data.
 */
proc_result_t ipc_asend(pid_t owner, index_t i, word_t data, word_t *status)
{
	if (!_ipc_invoke_valid_access(owner, i, IPC_MODE_ASYNC, false)) {
		*status = ERR_INVALID_ACCESS;
		return proc_resume_result(current);
	}

	index_t sink = ipc_table[i].sink;
	pid_t recv_pid = ipc_table[sink].owner;

	// Data stored in the opt field.
	ipc_table[sink].source = i;
	ipc_table[sink].opt = data;

	*status = ERR_SUCCESS;
	if ((ipc_table[i].flag & IPC_FLAG_YIELD) && recv_pid != INVALID_PID && proc_acquire(recv_pid)) {
		proc_t *receiver = proc_get(recv_pid);
		receiver->timeout = current->timeout;
		proc_release(owner);
		return proc_reload_result(receiver);
	}
	return proc_resume_result(current);
}

/**
 * Asynchronously receive data.
 */
int ipc_arecv(pid_t owner, index_t i, word_t *data)
{
	if (!_ipc_invoke_valid_access(owner, i, IPC_MODE_ASYNC, true)) {
		return ERR_INVALID_ACCESS;
	}
	// Read data from the opt field.
	*data = ipc_table[i].opt;
	return ERR_SUCCESS;
}

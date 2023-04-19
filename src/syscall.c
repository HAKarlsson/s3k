/* See LICENSE file for copyright and license details. */
#include "syscall.h"

#include "cap.h"
#include "common.h"
#include "consts.h"
#include "csr.h"
#include "kassert.h"
#include "schedule.h"
#include "ticket_lock.h"
#include "timer.h"
#include "trap.h"

static struct proc *_listeners[NCHANNEL];
static struct ticket_lock _lock;

static inline void syscall_lock(void)
{
	tl_acq(&_lock);
}

static inline void syscall_unlock(void)
{
	tl_rel(&_lock);
}

/*** System call handlers ***/
static inline uint64_t _getreg(struct proc *proc, uint64_t reg)
{
	return (reg < REG_COUNT) ? proc->regs[reg] : 0;
}

static inline void _setreg(struct proc *proc, uint64_t reg, uint64_t val)
{
	if (reg < REG_COUNT)
		proc->regs[reg] = val;
	if (reg == REG_PMP)
		proc_load_pmp(proc);
}

struct proc *syscall_proc(struct proc *proc, uint64_t a1, uint64_t a2,
			  uint64_t a3)
{
	switch (a1) {
	case 0: /* Get process ID */
		proc->regs[REG_A0] = proc->pid;
		break;
	case 1: /* Read register */
		proc->regs[REG_A0] = _getreg(proc, a2);
		break;
	case 2: /* Set register */
		_setreg(proc, a2, a3);
		break;
	case 3: /* Get HartID */
		proc->regs[REG_A0] = csrr_mhartid();
		break;
	case 4: /* Get RTC */
		proc->regs[REG_A0] = time_get();
		break;
	case 5: /* Get timeout */
		proc->regs[REG_A0] = timeout_get(csrr_mhartid());
		break;
	case 6: /* Yield the remaining time slice */
		proc->sleep = timeout_get(csrr_mhartid());
		return schedule_yield(proc);
	case 7: /* Suspend */
		proc_suspend(proc);
		return schedule_yield(proc);
	default:
		proc->regs[REG_A0] = 0;
		break;
	}
	return proc;
}

struct proc *syscall_getcap(struct proc *proc, uint64_t idx)
{
	cnode_handle_t handle = cnode_get_handle(proc->pid, idx);
	proc->regs[REG_A0] = cnode_get_cap(handle).raw;
	return proc;
}

struct proc *syscall_movcap(struct proc *proc, uint64_t srcIdx, uint64_t dstIdx)
{
	// Get handles
	cnode_handle_t src_handle = cnode_get_handle(proc->pid, srcIdx);
	cnode_handle_t dst_handle = cnode_get_handle(proc->pid, dstIdx);

	if (!cnode_contains(src_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	if (cnode_contains(dst_handle)) {
		proc->regs[REG_A0] = EXCPT_COLLISION;
		return proc;
	}
	syscall_lock();
	if (cnode_contains(src_handle)) {
		cnode_move(src_handle, dst_handle);
		proc->regs[REG_A0] = EXCPT_NONE;
	} else {
		proc->regs[REG_A0] = EXCPT_EMPTY;
	}
	syscall_unlock();
	return proc;
}

struct proc *syscall_delcap(struct proc *proc, uint64_t idx)
{
	cnode_handle_t handle = cnode_get_handle(proc->pid, idx);
	union cap cap = cnode_get_cap(handle);
	if (!cnode_contains(handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	syscall_lock();
	if (cnode_contains(handle)) {
		cnode_delete(handle);
		proc->regs[REG_A0] = EXCPT_NONE;
		if (cap.type == CAPTY_TIME) {
			schedule_delete(cap.time.hartid, cap.time.free,
					cap.time.end);
		}
	} else {
		proc->regs[REG_A0] = EXCPT_EMPTY;
	}
	syscall_unlock();
	return proc;
}

static void _revoke_time_hook(cnode_handle_t handle, union cap cap,
			      union cap child_cap)
{
	cap.time.free = child_cap.time.free;
	cnode_set_cap(handle, cap);
	schedule_update(cap.time.hartid, cnode_get_pid(handle), cap.time.free,
			cap.time.end);
}

static void _revoke_time_post_hook(cnode_handle_t handle, union cap cap)
{
	cap.time.free = cap.time.begin;
	cnode_set_cap(handle, cap);
	schedule_update(cap.time.hartid, cnode_get_pid(handle), cap.time.free,
			cap.time.end);
}

static void _revoke_memory_hook(cnode_handle_t handle, union cap cap,
				union cap child_cap)
{
	if (cap.type == CAPTY_MEMORY) {
		cap.memory.free = child_cap.memory.free; // Inherit free region
		cap.memory.lock = child_cap.memory.lock; // Inherit lock
		cnode_set_cap(handle, cap);
	}
}

static void _revoke_memory_post_hook(cnode_handle_t handle, union cap cap)
{
	cap.memory.free = cap.memory.begin;
	cap.memory.lock = 0;
	cnode_set_cap(handle, cap);
}

static void _revoke_monitor_hook(cnode_handle_t handle, union cap cap,
				 union cap child_cap)
{
	cap.monitor.free = child_cap.monitor.free;
	cnode_set_cap(handle, cap);
}

static void _revoke_monitor_post_hook(cnode_handle_t handle, union cap cap)
{
	cap.monitor.free = cap.monitor.begin;
	cnode_set_cap(handle, cap);
}

static void _revoke_channel_hook(cnode_handle_t handle, union cap cap,
				 union cap child_cap)
{
	if (child_cap.type == CAPTY_CHANNEL) {
		cap.channel.free = child_cap.channel.free;
		cnode_set_cap(handle, cap);
	} else if (child_cap.type == CAPTY_SOCKET
		   && child_cap.socket.tag == 0) {
		cap.channel.free = child_cap.socket.channel;
		_listeners[child_cap.socket.channel] = NULL;
	}
}

static void _revoke_channel_post_hook(cnode_handle_t handle, union cap cap)
{
	cap.channel.free = cap.channel.begin;
	cnode_set_cap(handle, cap);
}

static void _revoke_socket_hook(cnode_handle_t handle, union cap cap,
				union cap child_cap)
{
	/* This should be empty */
}

static void _revoke_socket_post_hook(cnode_handle_t handle, union cap cap)
{
	/* This should be empty */
}

struct proc *syscall_revcap(struct proc *proc, uint64_t idx)
{
	// Get handle
	cnode_handle_t handle = cnode_get_handle(proc->pid, idx);
	union cap cap = cnode_get_cap(handle);

	// If empty slot
	if (!cnode_contains(handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}

	bool (*is_parent)(union cap, union cap);
	void (*hook)(cnode_handle_t, union cap, union cap);
	void (*post_hook)(cnode_handle_t, union cap);
	switch (cap.type) {
	case CAPTY_TIME:
		is_parent = cap_time_parent;
		hook = _revoke_time_hook;
		post_hook = _revoke_time_post_hook;
		break;
	case CAPTY_MEMORY:
		is_parent = cap_memory_parent;
		hook = _revoke_memory_hook;
		post_hook = _revoke_memory_post_hook;
		break;
	case CAPTY_MONITOR:
		is_parent = cap_monitor_parent;
		hook = _revoke_monitor_hook;
		post_hook = _revoke_monitor_post_hook;
		break;
	case CAPTY_CHANNEL:
		is_parent = cap_channel_parent;
		hook = _revoke_channel_hook;
		post_hook = _revoke_channel_post_hook;
		break;
	case CAPTY_SOCKET:
		is_parent = cap_socket_parent;
		hook = _revoke_socket_hook;
		post_hook = _revoke_socket_post_hook;
		break;
	default:
		proc->regs[REG_A0] = EXCPT_NONE;
		return proc;
	}

	cnode_handle_t next_handle;
	union cap next_cap;
	while (cnode_contains(handle)) {
		next_handle = cnode_get_next(handle);
		if (next_handle == CNODE_ROOT_HANDLE)
			break;
		next_cap = cnode_get_cap(next_handle);
		if (!is_parent(cap, next_cap))
			break;
		syscall_lock();
		if (cnode_delete_if(next_handle, handle))
			hook(handle, cap, next_cap);
		syscall_unlock();
	}

	if (!cnode_contains(handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	syscall_lock();
	if (cnode_contains(handle)) {
		post_hook(handle, cap);
		proc->regs[REG_A0] = EXCPT_NONE;
	} else {
		proc->regs[REG_A0] = EXCPT_EMPTY;
	}
	syscall_unlock();
	return proc;
}

static void _derive_time(cnode_handle_t orig_handle, union cap orig_cap,
			 cnode_handle_t drv_handle, union cap drv_cap)
{
	orig_cap.time.free = drv_cap.time.end;
	cnode_set_cap(orig_handle, orig_cap);
	schedule_update(drv_cap.time.hartid, cnode_get_pid(orig_handle),
			drv_cap.time.begin, drv_cap.time.end);
}

static void _derive_memory(cnode_handle_t orig_handle, union cap orig_cap,
			   cnode_handle_t drv_handle, union cap drv_cap)
{
	if (drv_cap.type == CAPTY_MEMORY) { // Memory
		orig_cap.memory.free = drv_cap.memory.end;
	} else { // PMP
		orig_cap.memory.lock = true;
	}
	cnode_set_cap(orig_handle, orig_cap);
}

static void _derive_monitor(cnode_handle_t orig_handle, union cap orig_cap,
			    cnode_handle_t drv_handle, union cap drv_cap)
{
	orig_cap.monitor.free = drv_cap.monitor.end;
	cnode_set_cap(orig_handle, orig_cap);
}

static void _derive_channel(cnode_handle_t orig_handle, union cap orig_cap,
			    cnode_handle_t drv_handle, union cap drv_cap)
{
	// Update free pointer.
	if (drv_cap.type == CAPTY_CHANNEL) {
		orig_cap.channel.free = drv_cap.channel.end;
	} else {
		orig_cap.channel.free = drv_cap.socket.channel + 1;
		struct proc *receiver = proc_get(cnode_get_pid(orig_handle));
		_listeners[drv_cap.socket.channel] = receiver;
	}
	cnode_set_cap(orig_handle, orig_cap);
}

static void _derive_socket(cnode_handle_t orig_handle, union cap orig_cap,
			   cnode_handle_t drv_handle, union cap drv_cap)
{
	/* This should be empty */
}

struct proc *syscall_drvcap(struct proc *proc, uint64_t orig_idx,
			    uint64_t drv_idx, union cap drv_cap)
{
	// Get handle
	cnode_handle_t orig_handle = cnode_get_handle(proc->pid, orig_idx);
	cnode_handle_t drv_handle = cnode_get_handle(proc->pid, drv_idx);
	union cap orig_cap = cnode_get_cap(orig_handle);

	if (!cnode_contains(orig_handle)) { // If empty slot
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	} else if (cnode_contains(drv_handle)) { // If destination occupied
		proc->regs[REG_A0] = EXCPT_COLLISION;
		return proc;
	}

	void (*hook)(cnode_handle_t, union cap, cnode_handle_t, union cap);
	bool (*can_derive)(union cap, union cap);
	// Call specific handler for capability or return unimplemented.
	switch (orig_cap.type) {
	case CAPTY_TIME:
		hook = _derive_time;
		can_derive = cap_time_derive;
		break;
	case CAPTY_MEMORY:
		hook = _derive_memory;
		can_derive = cap_memory_derive;
		break;
	case CAPTY_MONITOR:
		hook = _derive_monitor;
		can_derive = cap_monitor_derive;
		break;
	case CAPTY_CHANNEL:
		hook = _derive_channel;
		can_derive = cap_channel_derive;
		break;
	case CAPTY_SOCKET:
		hook = _derive_socket;
		can_derive = cap_socket_derive;
		break;
	default:
		proc->regs[REG_A0] = EXCPT_UNIMPLEMENTED;
		return proc;
	}
	if (!can_derive(orig_cap, drv_cap)) {
		proc->regs[REG_A0] = EXCPT_DERIVATION;
	} else {
		syscall_lock();
		if (!cnode_contains(orig_handle)) {
			proc->regs[REG_A0] = EXCPT_EMPTY;
		} else {
			cnode_insert(drv_handle, drv_cap, orig_handle);
			hook(orig_handle, orig_cap, drv_handle, drv_cap);
			proc->regs[REG_A0] = EXCPT_NONE;
		}
		syscall_unlock();
	}
	return proc;
}

struct proc *syscall_msuspend(struct proc *proc, uint64_t mon_idx, uint64_t pid)
{
	cnode_handle_t mon_handle = cnode_get_handle(proc->pid, mon_idx);
	union cap cap = cnode_get_cap(mon_handle);

	if (!cnode_contains(mon_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	if (cap.type != CAPTY_MONITOR) {
		proc->regs[REG_A0] = EXCPT_UNIMPLEMENTED;
		return proc;
	}
	if (cap.monitor.free > pid || pid >= cap.monitor.end) {
		proc->regs[REG_A0] = EXCPT_MPID;
		return proc;
	}

	struct proc *other_proc = proc_get(pid);
	syscall_lock();
	if (!cnode_contains(mon_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	proc_suspend(other_proc);
	syscall_unlock();
	proc->regs[REG_A0] = EXCPT_NONE;
	return proc;
}

struct proc *syscall_mresume(struct proc *proc, uint64_t mon_idx, uint64_t pid)
{
	cnode_handle_t mon_handle = cnode_get_handle(proc->pid, mon_idx);
	union cap cap = cnode_get_cap(mon_handle);

	if (!cnode_contains(mon_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	if (cap.type != CAPTY_MONITOR) {
		proc->regs[REG_A0] = EXCPT_UNIMPLEMENTED;
		return proc;
	}
	if (cap.monitor.free > pid || pid >= cap.monitor.end) {
		proc->regs[REG_A0] = EXCPT_MPID;
		return proc;
	}

	struct proc *other_proc = proc_get(pid);
	syscall_lock();
	if (!cnode_contains(mon_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	proc_resume(other_proc);
	syscall_unlock();
	proc->regs[REG_A0] = EXCPT_NONE;
	return proc;
}

struct proc *syscall_mgetreg(struct proc *proc, uint64_t mon_idx, uint64_t pid,
			     uint64_t reg)
{
	cnode_handle_t mon_handle = cnode_get_handle(proc->pid, mon_idx);
	union cap cap = cnode_get_cap(mon_handle);
	if (!cnode_contains(mon_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	if (cap.type != CAPTY_MONITOR) {
		proc->regs[REG_A0] = EXCPT_UNIMPLEMENTED;
		return proc;
	}
	if (cap.monitor.free > pid || pid >= cap.monitor.end) {
		proc->regs[REG_A0] = EXCPT_MPID;
		return proc;
	}
	syscall_lock();
	if (!cnode_contains(mon_handle)) {
		syscall_unlock();
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}

	struct proc *other_proc = proc_get(pid);
	if (!proc_acquire(other_proc, PS_SUSPENDED)) {
		syscall_unlock();
		proc->regs[REG_A0] = EXCPT_MBUSY;
		return proc;
	}
	proc->regs[REG_A1] = other_proc->regs[reg % REG_COUNT];
	proc_release(other_proc);
	__sync_fetch_and_and(&other_proc->state, ~PSF_BUSY);
	syscall_unlock();
	proc->regs[REG_A0] = EXCPT_NONE;
	return proc;
}

struct proc *syscall_msetreg(struct proc *proc, uint64_t mon_idx, uint64_t pid,
			     uint64_t reg, uint64_t val)
{
	cnode_handle_t mon_handle = cnode_get_handle(proc->pid, mon_idx);
	union cap cap = cnode_get_cap(mon_handle);
	if (!cnode_contains(mon_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	if (cap.type != CAPTY_MONITOR) {
		proc->regs[REG_A0] = EXCPT_UNIMPLEMENTED;
		return proc;
	}
	if (cap.monitor.free > pid || pid >= cap.monitor.end) {
		proc->regs[REG_A0] = EXCPT_MPID;
		return proc;
	}

	syscall_lock();
	if (!cnode_contains(mon_handle)) {
		syscall_unlock();
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	struct proc *other_proc = proc_get(pid);
	if (!proc_acquire(other_proc, PS_SUSPENDED)) {
		syscall_unlock();
		proc->regs[REG_A0] = EXCPT_MBUSY;
		return proc;
	}
	other_proc->regs[reg % REG_COUNT] = val;
	proc_release(other_proc);
	syscall_unlock();
	proc->regs[REG_A0] = EXCPT_NONE;
	return proc;
}

struct proc *syscall_mgetcap(struct proc *proc, uint64_t mon_idx, uint64_t pid,
			     uint64_t node_idx)
{
	cnode_handle_t mon_handle = cnode_get_handle(proc->pid, mon_idx);
	union cap cap = cnode_get_cap(mon_handle);
	if (!cnode_contains(mon_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	if (cap.type != CAPTY_MONITOR) {
		proc->regs[REG_A0] = EXCPT_UNIMPLEMENTED;
		return proc;
	}
	if (cap.monitor.free > pid || pid >= cap.monitor.end) {
		proc->regs[REG_A0] = EXCPT_MPID;
		return proc;
	}
	syscall_lock();
	if (!cnode_contains(mon_handle)) {
		syscall_unlock();
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}

	struct proc *other_proc = proc_get(pid);
	if (!proc_acquire(other_proc, PS_SUSPENDED)) {
		syscall_unlock();
		proc->regs[REG_A0] = EXCPT_MBUSY;
		return proc;
	}
	cnode_handle_t node_handle = cnode_get_handle(pid, node_idx);
	proc->regs[REG_A1] = cnode_get_cap(node_handle).raw;
	proc_release(other_proc);
	syscall_unlock();
	proc->regs[REG_A0] = EXCPT_NONE;
	return proc;
}

struct proc *syscall_mtakecap(struct proc *proc, uint64_t mon_idx, uint64_t pid,
			      uint64_t src_idx, uint64_t dst_idx)
{
	cnode_handle_t mon_handle = cnode_get_handle(proc->pid, mon_idx);
	union cap cap = cnode_get_cap(mon_handle);
	if (!cnode_contains(mon_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	if (cap.type != CAPTY_MONITOR) {
		proc->regs[REG_A0] = EXCPT_UNIMPLEMENTED;
		return proc;
	}
	if (cap.monitor.free > pid || pid >= cap.monitor.end) {
		proc->regs[REG_A0] = EXCPT_MPID;
		return proc;
	}
	syscall_lock();
	if (!cnode_contains(mon_handle)) {
		syscall_unlock();
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	struct proc *other_proc = proc_get(pid);
	if (!proc_acquire(other_proc, PS_SUSPENDED)) {
		syscall_unlock();
		proc->regs[REG_A0] = EXCPT_MBUSY;
		return proc;
	}
	cnode_handle_t src_handle = cnode_get_handle(pid, src_idx);
	cnode_handle_t dst_handle = cnode_get_handle(proc->pid, dst_idx);
	if (cnode_contains(src_handle) && !cnode_contains(dst_handle)) {
		union cap src_cap = cnode_get_cap(src_handle);
		cnode_move(src_handle, dst_handle);
		if (src_cap.type == CAPTY_TIME) {
			schedule_update(src_cap.time.hartid, proc->pid,
					src_cap.time.free, src_cap.time.end);
		}
		if (src_cap.type == CAPTY_SOCKET && src_cap.socket.tag == 0) {
			_listeners[src_cap.socket.channel] = proc;
		}
	}
	proc_release(other_proc);
	syscall_unlock();
	proc->regs[REG_A0] = EXCPT_NONE;
	return proc;
}

struct proc *syscall_mgivecap(struct proc *proc, uint64_t mon_idx, uint64_t pid,
			      uint64_t src_idx, uint64_t dst_idx)
{
	cnode_handle_t mon_handle = cnode_get_handle(proc->pid, mon_idx);
	union cap cap = cnode_get_cap(mon_handle);
	if (!cnode_contains(mon_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	if (cap.type != CAPTY_MONITOR) {
		proc->regs[REG_A0] = EXCPT_UNIMPLEMENTED;
		return proc;
	}
	if (cap.monitor.free > pid || pid >= cap.monitor.end) {
		proc->regs[REG_A0] = EXCPT_MPID;
		return proc;
	}
	syscall_lock();
	if (!cnode_contains(mon_handle)) {
		syscall_unlock();
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}

	struct proc *other_proc = proc_get(pid);
	if (!proc_acquire(other_proc, PS_SUSPENDED)) {
		syscall_unlock();
		proc->regs[REG_A0] = EXCPT_MBUSY;
		return proc;
	}

	cnode_handle_t src_handle = cnode_get_handle(proc->pid, src_idx);
	cnode_handle_t dst_handle = cnode_get_handle(pid, dst_idx);
	if (cnode_contains(src_handle) && !cnode_contains(dst_handle)) {
		union cap src_cap = cnode_get_cap(src_handle);
		cnode_move(src_handle, dst_handle);
		if (src_cap.type == CAPTY_TIME) {
			schedule_update(src_cap.time.hartid, pid,
					src_cap.time.free, src_cap.time.end);
		}
		if (src_cap.type == CAPTY_SOCKET && src_cap.socket.tag == 0) {
			_listeners[src_cap.socket.channel] = other_proc;
		}
	}
	proc_release(other_proc);
	syscall_unlock();
	proc->regs[REG_A0] = EXCPT_NONE;
	return proc;
}

struct proc *syscall_recv(struct proc *proc, uint64_t recv_idx,
			  uint64_t cap_dest)
{
	cnode_handle_t recv_cap_handle = cnode_get_handle(proc->pid, recv_idx);
	union cap recv_cap = cnode_get_cap(recv_cap_handle);
	if (!cnode_contains(recv_cap_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	if (recv_cap.type != CAPTY_SOCKET || recv_cap.socket.tag != 0) {
		proc->regs[REG_A0] = EXCPT_INVALID_CAP;
		return proc;
	}
	uint64_t channel = recv_cap.socket.channel;
	syscall_lock();
	if (!cnode_contains(recv_cap_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		syscall_unlock();
		return proc;
	}

	kassert(_listeners[channel] == proc);

	proc->regs[REG_A0] = EXCPT_PREEMPTED;
	proc->cap_dest = cap_dest;
	if (proc_ipc_wait(proc, channel)) {
		// Waiting success
		syscall_unlock();
		return schedule_next();
	} else {
		// Waiting failed, process is suspended.
		syscall_unlock();
		return schedule_yield(proc);
	}
}

struct proc *syscall_send(struct proc *proc, uint64_t send_idx, uint64_t msg0,
			  uint64_t msg1, uint64_t msg2, uint64_t msg3,
			  uint64_t cap_src, uint64_t yield)
{
	cnode_handle_t send_cap_handle = cnode_get_handle(proc->pid, send_idx);
	union cap send_cap = cnode_get_cap(send_cap_handle);
	if (!cnode_contains(send_cap_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		return proc;
	}
	if (send_cap.type != CAPTY_SOCKET || send_cap.socket.tag == 0) {
		proc->regs[REG_A0] = EXCPT_INVALID_CAP;
		return proc;
	}
	uint64_t channel = send_cap.socket.channel;
	struct proc *receiver = _listeners[channel];
	if (receiver == NULL || !proc_ipc_acquire(receiver, channel)) {
		proc->regs[REG_A0] = EXCPT_NO_RECEIVER;
		return proc;
	}

	syscall_lock();
	if (!cnode_contains(send_cap_handle)) {
		proc->regs[REG_A0] = EXCPT_EMPTY;
		syscall_unlock();
		return proc;
	}
	// Check if we can send a capability.
	uint64_t cap_dest = receiver->cap_dest;
	cnode_handle_t dest_handle = cnode_get_handle(receiver->pid, cap_dest);
	cnode_handle_t src_handle = cnode_get_handle(proc->pid, cap_src);
	if (cap_src != -1ull) {
		if (cnode_contains(dest_handle)
		    || !cnode_contains(src_handle)) {
			proc->regs[REG_A0] = EXCPT_SEND_CAP;
			syscall_unlock();
			return proc;
		}
		union cap cap = cnode_get_cap(src_handle);
		cnode_move(src_handle, dest_handle);
		if (cap.type == CAPTY_TIME) {
			schedule_update(cap.time.hartid, receiver->pid,
					cap.time.free, cap.time.end);
		}
		if (cap.type == CAPTY_SOCKET && cap.socket.tag == 0) {
			_listeners[cap.socket.channel] = receiver;
		}
	}
	receiver->regs[REG_A0] = EXCPT_NONE;
	receiver->regs[REG_A1] = msg0;
	receiver->regs[REG_A2] = msg1;
	receiver->regs[REG_A3] = msg2;
	receiver->regs[REG_A4] = msg3;
	receiver->regs[REG_A5] = send_cap.socket.tag;
	proc_release(receiver);
	proc->regs[REG_A0] = EXCPT_NONE;
	syscall_unlock();
	if (yield)
		return schedule_yield(proc);
	return proc;
}

struct proc *syscall_sendrecv(struct proc *proc, uint64_t sendrecv_idx,
			      uint64_t msg0, uint64_t msg1, uint64_t msg2,
			      uint64_t msg3, uint64_t send_cap,
			      uint64_t dest_cap)
{
	syscall_send(proc, sendrecv_idx >> 16, msg0, msg1, msg2, msg3, send_cap,
		     false);
	return syscall_recv(proc, sendrecv_idx & 0xFFFF, dest_cap);
}

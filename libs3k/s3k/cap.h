#ifndef S3K_CAP_H
#define S3K_CAP_H

#include <stdint.h>

typedef enum {
	S3K_MEM_NONE = 0,
	S3K_MEM_R = 1,
	S3K_MEM_W = 2,
	S3K_MEM_X = 4,
	S3K_MEM_RW = S3K_MEM_R | S3K_MEM_W,
	S3K_MEM_RX = S3K_MEM_R | S3K_MEM_X,
	S3K_MEM_RWX = S3K_MEM_R | S3K_MEM_W | S3K_MEM_X,
} s3k_mem_perm_t;

// IPC Modes
typedef enum {
	S3K_IPC_NOYIELD = 0, // Non-Yielding Synchronous
	S3K_IPC_YIELD = 1,   // Yielding Synchronous
	S3K_IPC_ASYNC = 2,   // Asynchronous
} s3k_ipc_mode_t;

// IPC Permissions
typedef enum {
	S3K_IPC_SDATA = 1, // Server can send data
	S3K_IPC_SCAP = 2,  // Server can send capabilities
	S3K_IPC_CDATA = 4, // Client can send data
	S3K_IPC_CCAP = 8,  // Client can send capabilities
} s3k_ipc_perm_t;

// Capability types
typedef enum s3k_capty {
	S3K_CAPTY_NONE = 0,    ///< No capability.
	S3K_CAPTY_TIME = 1,    ///< Time Slice capability.
	S3K_CAPTY_MEMORY = 2,  ///< Memory Slice capability.
	S3K_CAPTY_PMP = 3,     ///< PMP Frame capability.
	S3K_CAPTY_MONITOR = 4, ///< Monitor capability.
	S3K_CAPTY_CHANNEL = 5, ///< IPC Channel capability.
	S3K_CAPTY_SOCKET = 6,  ///< IPC Socket capability.
} s3k_capty_t;

/// Capability description
typedef union s3k_cap {
	uint64_t type : 4;
	uint64_t raw;

	struct {
		uint64_t type : 4;
		uint64_t unused : 4;
		uint64_t hart : 8;
		uint64_t base : 16;
		uint64_t alloc : 16;
		uint64_t size : 16;
	} time;

	struct {
		uint64_t type : 4;
		uint64_t rwx : 3;
		uint64_t lock : 1;
		uint64_t base : 24;
		uint64_t alloc : 16;
		uint64_t size : 16;
	} memory;

	struct {
		uint64_t type : 4;
		uint64_t rwx : 3;
		uint64_t used : 1;
		uint64_t index : 8;
		uint64_t addr : 48;
	} pmp;

	struct {
		uint64_t type : 4;
		uint64_t unused : 12;
		uint64_t base : 16;
		uint64_t alloc : 16;
		uint64_t size : 16;
	} monitor;

	struct {
		uint64_t type : 4;
		uint64_t unused : 12;
		uint64_t base : 16;
		uint64_t alloc : 16;
		uint64_t size : 16;
	} channel;

	struct {
		uint64_t type : 4;
		uint64_t mode : 4;
		uint64_t perm : 8;
		uint64_t channel : 16;
		uint64_t tag : 32;
	} socket;
} s3k_cap_t;

/**
 * Construct a time slice capability.
 *
 * @param hart  Hardware thread ID.
 * @param base  Base of the time slice.
 * @param size  Size of the time slice.
 * @return      Initialized time capability.
 */
static inline s3k_cap_t s3k_time_cap(uint64_t hart, uint64_t base,
				     uint64_t size)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_TIME;
	cap.time.base = base;
	cap.time.alloc = 0;
	cap.time.size = size;
	return cap;
}

/**
 * Construct a memory slice capability.
 *
 * @param base  Base memory address in 4KB.
 * @param size  Size of the memory in 4KB.
 * @param rwx   Read-write-execute permissions encoded as bit flags.
 * @return      Initialized memory capability.
 */
static inline s3k_cap_t s3k_memory_cap(uint64_t base, uint64_t size,
				       uint64_t rwx)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_MEMORY;
	cap.memory.base = base;
	cap.memory.alloc = 0;
	cap.memory.size = size;
	cap.memory.rwx = rwx;
	cap.memory.lock = 0;
	return cap;
}

/**
 * Construct a PMP (Physical Memory Protection) capability using NAPOT mode.
 *
 * @param addr  Base address of the naturally aligned power-of-two region. The
 * alignment should adhere to the constraints set by NAPOT mode.
 * @param rwx   Read-write-execute permissions encoded as bit flags for the
 * region.
 * @return      Initialized PMP capability.
 */
static inline s3k_cap_t s3k_pmp_cap(uint64_t addr, uint64_t rwx)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_PMP;
	cap.pmp.addr = addr;
	cap.pmp.rwx = rwx;
	cap.pmp.used = 0;
	cap.pmp.index = 0;
	return cap;
}

/**
 * Construct a monitor slice capability.
 *
 * @param base   Base of for the monitor.
 * @param size   Size of the monitor capability.
 * @return       Initialized monitor capability.
 */
static inline s3k_cap_t s3k_monitor_cap(uint64_t base, uint64_t size)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_MONITOR;
	cap.monitor.base = base;
	cap.monitor.size = size;
	return cap;
}

/**
 * Construct an IPC channel slice capability.
 *
 * @param base   Base of the IPC channel.
 * @param size   Size of the IPC channel capability.
 * @return       Initialized IPC channel capability.
 */
static inline s3k_cap_t s3k_channel_cap(uint64_t base, uint64_t size)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_CHANNEL;
	cap.channel.base = base;
	cap.channel.size = size;
	return cap;
}

/**
 * Construct an IPC socket capability.
 *
 * Sockets are of two main types: server and client. Server sockets have a tag
 * of 0, while client sockets have a non-zero tag. Depending on the mode
 * (yielding, non-yielding, or asynchronous), there can be a transfer or
 * donation of execution time. Specifically:
 * - Yielding: Upon sending a message, the sender donates its remaining
 * execution time to the receiver.
 * - Non-Yielding: Standard communication without time donation.
 * - Asynchronous: The client donates its execution time to the server
 * asynchronously.
 *
 * @param channel  Channel identifier or address for the IPC socket.
 * @param mode     Communication mode for the IPC socket.
 * @param perm     Permissions associated with the IPC socket, determining the
 * behavior of both server and client:
 *                 - SERVER_DATA: Server can send data, and the client is
 * expected to receive data.
 *                 - SERVER_CAP: Server can send capabilities, and the client is
 * expected to receive capabilities.
 *                 - CLIENT_DATA: Client can send data, and the server is
 * expected to receive data.
 *                 - CLIENT_CAP: Client can send capabilities, and the server is
 * expected to receive capabilities.
 * @param tag      Tag or type identifier for the IPC socket. A tag of 0
 * indicates a server socket, whereas a non-zero tag indicates a client socket.
 * @return         Initialized IPC socket capability.
 */
static inline s3k_cap_t s3k_socket_cap(uint64_t channel, uint64_t mode,
				       uint64_t perm, uint64_t tag)
{
	s3k_cap_t cap;
	cap.type = S3K_CAPTY_SOCKET;
	cap.socket.channel = channel;
	cap.socket.mode = mode;
	cap.socket.perm = perm;
	cap.socket.tag = tag;
	return cap;
}

#endif

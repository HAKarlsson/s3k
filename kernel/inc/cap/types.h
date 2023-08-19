#ifndef CAP_TYPES_H
#define CAP_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	MEM_NONE = 0,
	MEM_R = 1,
	MEM_W = 2,
	MEM_X = 4,
	MEM_RW = MEM_R | MEM_W,
	MEM_RX = MEM_R | MEM_X,
	MEM_RWX = MEM_R | MEM_W | MEM_X,
} mem_perm_t;

// IPC Modes
typedef enum {
	IPC_NOYIELD = 0, // Non-Yielding Synchronous
	IPC_YIELD = 1,	 // Yielding Synchronous
	IPC_ASYNC = 2,	 // Asynchronous
} ipc_mode_t;

// IPC Permissions
typedef enum {
	IPC_SDATA = 1, // Server can send data
	IPC_SCAP = 2,  // Server can send capabilities
	IPC_CDATA = 4, // Client can send data
	IPC_CCAP = 8,  // Client can send capabilities
} ipc_perm_t;

// Capability types
typedef enum capty {
	CAPTY_NONE = 0,	   ///< No capability.
	CAPTY_TIME = 1,	   ///< Time Slice capability.
	CAPTY_MEMORY = 2,  ///< Memory Slice capability.
	CAPTY_PMP = 3,	   ///< PMP Frame capability.
	CAPTY_MONITOR = 4, ///< Monitor capability.
	CAPTY_CHANNEL = 5, ///< IPC Channel capability.
	CAPTY_SERVER = 6,  ///< IPC Server capability.
	CAPTY_CLIENT = 7,  ///< IPC Client capability.
} capty_t;

/// Capability description
typedef union cap {
	uint64_t type : 4;
	uint64_t raw;

	struct {
		uint64_t type : 4;
		uint64_t unused : 4;
		uint64_t hart : 8;
		uint64_t begin : 16;
		uint64_t mark : 16;
		uint64_t end : 16;
	} time;

	struct {
		uint64_t type : 4;
		uint64_t rwx : 3;
		uint64_t lock : 1;
		uint64_t offset : 8;
		uint64_t begin : 16;
		uint64_t mark : 16;
		uint64_t end : 16;
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
		uint64_t begin : 16;
		uint64_t mark : 16;
		uint64_t end : 16;
	} monitor;

	struct {
		uint64_t type : 4;
		uint64_t unused : 12;
		uint64_t begin : 16;
		uint64_t mark : 16;
		uint64_t end : 16;
	} channel;

	struct {
		uint64_t type : 4;
		uint64_t mode : 4;
		uint64_t perm : 8;
		uint64_t channel : 16;
		uint64_t unused : 32;
	} server;

	struct {
		uint64_t type : 4;
		uint64_t mode : 4;
		uint64_t perm : 8;
		uint64_t channel : 16;
		uint64_t tag : 32;
	} client;
} cap_t;

cap_t time_cap(uint64_t hart, uint64_t begin, uint64_t eind);
cap_t memory_cap(uint64_t begin, uint64_t end, uint64_t rwx);
cap_t pmp_cap(uint64_t addr, uint64_t rwx);
cap_t monitor_cap(uint64_t begin, uint64_t end);
cap_t channel_cap(uint64_t begin, uint64_t end);
cap_t socket_cap(uint64_t channel, uint64_t mode, uint64_t perm, uint64_t tag);
bool cap_can_revoke(cap_t, cap_t);
bool cap_can_derive(cap_t, cap_t);

#endif /* CAP_TYPES_H */

#pragma once

/// System call exceptions
typedef enum {
	EXCPT_NONE,	   ///< No exception.
	EXCPT_ERROR,	   ///< Some error.
	EXCPT_INDEX,	   ///< Index out-of-bounds.
	EXCPT_EMPTY,	   ///< Capability slot is empty.
	EXCPT_COLLISION,   ///< Capability slot is occupied.
	EXCPT_DERIVATION,  ///< Capability can not be derived.
	EXCPT_INVALID_CAP, ///< Capability used for the system call is invalid.
	EXCPT_REG_INDEX,
	EXCPT_PMP_INDEX,
	EXCPT_PMP_COLLISION,
	EXCPT_MONITOR_PID,	 ///< Bad PID for monitor operation.
	EXCPT_MONITOR_BUSY,	 ///< Process busy.
	EXCPT_MONITOR_INDEX,	 ///< Source or destination invalid.
	EXCPT_MONITOR_REG_INDEX, ///< Source or destination invalid.
	EXCPT_MONITOR_EMPTY,	 ///< Source is empty.
	EXCPT_MONITOR_COLLISION, ///< Destination is occupied.
	EXCPT_UNIMPLEMENTED,
	EXCPT_PREEMPT = -1
} excpt_t;

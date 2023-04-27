#ifndef __EXCPT_H__
#define __EXCPT_H__

/// System call exceptions
typedef enum {
	EXCPT_NONE,	   ///< No exception.
	EXCPT_INDEX,	   ///< Index out-of-bounds
	EXCPT_EMPTY,	   ///< Capability slot is empty.
	EXCPT_COLLISION,   ///< Capability slot is occupied.
	EXCPT_DERIVATION,  ///< Capability can not be derived.
	EXCPT_PREEMPTED,   ///< System call was preempted.
	EXCPT_SUSPENDED,   ///< Process was suspended
	EXCPT_MPID,	   ///< Bad PID for monitor operation.
	EXCPT_MBUSY,	   ///< Process busy.
	EXCPT_INVALID_CAP, ///< Capability used for the system call is invalid.
	EXCPT_NO_RECEIVER, ///< No receiver for the send call.
	EXCPT_SEND_CAP,	   ///< Something stops sending of capability.
	EXCPT_UNIMPLEMENTED, ///< System call not implemented for specified
			     ///< capability.
	EXCPT_UNSPECIFIED    ///< Unspecified error
} excpt_t;

#endif /* __EXCPT_H__ */

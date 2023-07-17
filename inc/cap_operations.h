#pragma once

#include "cap_table.h"
#include "cap_types.h"

int cap_get_cap(cptr_t, uint64_t[1]);

/**
 * Moves a capability from the source capability pointer to the destination capability pointer.
 *
 * @param src The source capability pointer.
 * @param dest The destination capability pointer.
 * @return Exception code indicating the success or failure of the move operation.
 * @note The source and destination capability pointers must belong to the same process.
 */
int cap_move(cptr_t src, cptr_t dest);

/**
 * Deletes a capability associated with the provided capability pointer.
 *
 * @param cptr The capability pointer.
 * @return Exception code indicating the success or failure of the delete operation.
 */
int cap_delete(cptr_t cptr);

/**
 * Revokes a capability and its children starting from the provided capability pointer.
 *
 * @param cptr The capability pointer to start the revocation from.
 * @return Exception code indicating the success or failure of the revocation operation.
 */
int cap_revoke(cptr_t cptr);

/**
 * Derives a new capability from the original capability and assigns it to the destination capability pointer.
 *
 * @param orig The original capability pointer.
 * @param dest The destination capability pointer.
 * @param new_cap The new capability to derive from the original capability.
 * @return Exception code indicating the success or failure of the derivation operation.
 */
int cap_derive(cptr_t orig, cptr_t dest, cap_t new_cap);

/**
 * Sets the Physical Memory Protection (PMP) configuration at the specified index
 * using the capability associated with the provided capability pointer.
 *
 * @param cptr The capability pointer.
 * @param index The index of the PMP configuration.
 * @return Exception code indicating the success or failure of the PMP set operation.
 */
int cap_pmp_set(cptr_t cptr, uint64_t index);

/**
 * Clears the Physical Memory Protection (PMP) configuration using the capability
 * associated with the provided capability pointer.
 *
 * @param cptr The capability pointer.
 * @return Exception code indicating the success or failure of the PMP clear operation.
 */
int cap_pmp_clear(cptr_t cptr);

/**
 * Suspends the execution of a process with the specified process ID using
 * the capability associated with the provided capability pointer.
 *
 * @param mon_cptr The monitor capability pointer.
 * @param pid The process ID to suspend.
 * @return Exception code indicating the success or failure of the process suspension.
 */
int cap_monitor_suspend(cptr_t cptr, uint64_t pid);

/**
 * Resumes the execution of a process with the specified process ID using
 * the capability associated with the provided capability pointer.
 *
 * @param mon_cptr The monitor capability pointer.
 * @param pid The process ID to resume.
 * @return Exception code indicating the success or failure of the process resumption.
 */
int cap_monitor_resume(cptr_t mon_cptr, uint64_t pid);

/**
 * Retrieves the value of a register from the process associated with the provided capability pointer.
 *
 * @param mon_cptr The monitor capability pointer.
 * @param pid The process ID associated with the monitor capability.
 * @param reg The register index to retrieve.
 * @param ret Array to store the retrieved register value.
 * @return Exception code indicating the success or failure of the register retrieval.
 */
int cap_monitor_get_reg(cptr_t mon_cptr, uint64_t pid, uint64_t reg, uint64_t ret[1]);

/**
 * Sets the value of a register in the process associated with the provided capability pointer.
 *
 * @param mon_cptr The monitor capability pointer.
 * @param pid The process ID associated with the monitor capability.
 * @param reg The register index to set.
 * @param val The value to set in the register.
 * @return Exception code indicating the success or failure of the register setting.
 */
int cap_monitor_set_reg(cptr_t mon_cptr, uint64_t pid, uint64_t reg, uint64_t val);
/**
 * Retrieves the capability descriptor from the process associated with the provided capability pointer.
 *
 * @param mon_cptr The monitor capability pointer.
 * @param pid The process ID associated with the monitor capability.
 * @param cidx The index of the capability descriptor we want to read.
 * @param ret Array to store the raw capability descriptor retrieved from the process.
 * @return Exception code indicating the success or failure of the function call itself.
 */
int cap_monitor_get_cap(cptr_t mon_cptr, uint64_t pid, cptr_t cptr, uint64_t ret[1]);

/**
 * Moves a capability from the source capability pointer to the destination capability pointer
 * within the context of a monitor capability.
 *
 * @param mon_cptr The monitor capability pointer.
 * @param pid The process ID associated with the monitor capability.
 * @param src_cidx The index of the source capability descriptor.
 * @param dest_cidx The index of the destination capability descriptor.
 * @param take Flag indicating whether to perform a take capability operation.
 * @return Exception code indicating the success or failure of the capability move operation.
 */
int cap_monitor_move_cap(cptr_t mon_cptr, uint64_t pid, cptr_t src_cidx, cptr_t dest_cidx);

/** TODO COMMENT */
int cap_monitor_pmp_set(cptr_t mon_cptr, uint64_t pid, cptr_t pmp_cptr, uint64_t index);

/** TODO COMMENT */
int cap_monitor_pmp_clear(cptr_t mon_cptr, uint64_t pid, cptr_t pmp_cptr);

/** TODO COMMENT */
int cap_socket_send(cptr_t sock_cptr, uint64_t buf[4], cptr_t buf_cptr);

/** TODO COMMENT */
int cap_socket_recv(cptr_t sock_cptr);

/** TODO COMMENT */
int cap_socket_sendrecv(cptr_t sock_cptr, uint64_t buf[4], cptr_t buf_cptr);

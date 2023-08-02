#pragma once
/**
 * @file proc.h
 * @brief Defines the process control block and its associated functions.
 *
 * This file contains the definition of the `proc_t` data structure, which
 * represents a process control block (PCB) in the operating system. It also
 * contains the declarations of functions for manipulating the PCB.
 *
 * @copyright MIT License
 */

#include "consts.h"
#include "ticket_lock.h"

#include <stdbool.h>
#include <stdint.h>

/** Process state flags
 * PSF_BUSY: Process has been acquired.
 * PSF_BLOCKED: Waiting for IPC.
 * PSF_SUSPENDED: Waiting for monitor
 */
#define PSF_BUSY 1
#define PSF_BLOCKED 2
#define PSF_SUSPENDED 4

/**
 * @brief Process control block.
 *
 * Contains all information needed manage a process except the capabilities.
 */
typedef struct {
	/** The registers of the process (RISC-V registers and virtual
	 * registers). */
	uint64_t regs[N_REGS];
	/** PMP registers */
	uint8_t pmpcfg[N_PMP];
	uint64_t pmpaddr[N_PMP];
	/** Instrumentation registers */
	uint64_t instrument_wcet;
	/** Process ID. */
	uint64_t pid;
	/** Process state. */
	uint64_t state;
	/** Sleep until. */
	uint64_t sleep;
	uint64_t start_time;
	uint64_t end_time;
} proc_t;

/**
 * Initializes all processes in the system.
 *
 * @param payload A pointer to the boot loader's code.
 *
 * @note This function should be called only once during system startup.
 */
void proc_init(void);

/**
 * @brief Gets the process corresponding to a given process ID.
 *
 * @param pid The process ID to look for.
 * @return A pointer to the process corresponding to the given PID.
 */
proc_t *proc_get(uint64_t pid);

/**
 * @brief Attempt to acquire the lock for a process.
 *
 * The process's lock is embedded in its state. This function attempts to
 * acquire the lock by atomically setting the LSB of the state to 1 if it
 * currently has the value 'expected'. If the lock is already held by another
 * process, this function will return false.
 *
 * @param proc Pointer to the process to acquire the lock for.
 * @param expected The expected value of the process's state.
 * @return True if the lock was successfully acquired, false otherwise.
 */
bool proc_acquire(proc_t *proc);

bool proc_monitor_acquire(proc_t *proc);

/**
 * @brief Release the lock on a process.
 *
 * The process's lock is embedded in its state. This function sets the LSB of
 * the state to 0 to unlock the process.
 *
 * @param proc Pointer to the process to release the lock for.
 */
void proc_release(proc_t *proc);

/**
 * Set the process to a suspended state without locking it. The process may
 * still be running, but it will not resume after its timeslice has ended.
 *
 * @param proc Pointer to process to suspend.
 */
void proc_suspend(proc_t *proc);

/**
 * Resumes a process from its suspend state without locking it.
 *
 * @param proc Pointer to process to be resumed.
 */
void proc_resume(proc_t *proc);

/**
 * The process is set to wait on a channel atomically if it is not ordered to
 * suspend. After begin set to wait, schedule_next() should be called. If
 * ordered to suspend, schedule_yield() should be called.
 */
bool proc_ipc_wait(proc_t *proc, uint64_t channel_id);

/**
 * The process is waiting for an IPC send, the channel it is waiting on is
 * included in its state. This function will atomically acquire the process
 * if its state is waiting on the provided channel id. The processes is
 * released with proc_ipc_release().
 */
bool proc_ipc_acquire(proc_t *proc, uint64_t channel_id);
void proc_ipc_release(proc_t *proc);

void proc_pmp_set(proc_t *proc, uint64_t index, uint64_t addr, uint64_t rwx);
void proc_pmp_clear(proc_t *proc, uint64_t index);
bool proc_pmp_is_set(proc_t *proc, uint64_t index);
void proc_pmp_load(proc_t *proc);
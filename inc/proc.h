/**
 * @file proc.h
 * @brief Defines the process control block and its associated functions.
 *
 * This file contains the definition of the `proc_t` data structure, which
 * represents a process control block (PCB) in the operating system. It also
 * contains the declarations of functions for manipulating the PCB.
 *
 * @copyright MIT License
 * @author Henrik Karlsson (henrik10@kth.se)
 */
#ifndef __PROC_H__
#define __PROC_H__

#include "ticket_lock.h"

#include <stdbool.h>
#include <stdint.h>

#define PMP_COUNT 8

typedef struct regs {
	uint64_t pc;
	uint64_t ra, sp, gp, tp;
	uint64_t t0, t1, t2;
	uint64_t s0, s1;
	uint64_t a0, a1, a2, a3, a4, a5, a6, a7;
	uint64_t s2, s3, s4, s5, s6, s7, s8, s9;
	uint64_t s10, s11;
	uint64_t t3, t4, t5, t6;
	uint64_t tpc, tsp;
	uint64_t epc, esp;
	uint64_t ecause, eval;
} regs_t;

#define REG_COUNT (sizeof(struct regs) / sizeof(uint64_t))

/**
 * @brief Process control block.
 *
 * Contains all information needed manage a process except the capabilities.
 */
typedef struct {
	/** The registers of the process (RISC-V registers and virtual
	 * registers). */
	regs_t regs;
	/** PMP settings */
	uint8_t pmpcfg[PMP_COUNT];
	uint64_t pmpaddr[PMP_COUNT];
	/** Process ID. */
	uint64_t pid;
	/** Process state. */
	uint64_t state;
	/** Sleep until. */
	uint64_t sleep;
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
bool proc_acquire(proc_t *proc, uint64_t expected);

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

/**
 * @brief Loads the PMP settings of the process to the hardware.
 *
 * This function loads the PMP settings of the process to the hardware. The PMP
 * settings specify the memory regions that the process can access. This
 * function loads the PMP settings to the hardware so that the hardware enforces
 * the process's memory access permissions.
 *
 * @param proc Pointer to the process for which we load PMP settings.
 */
void proc_load_pmp(const proc_t *proc);

#endif /* __PROC_H__ */

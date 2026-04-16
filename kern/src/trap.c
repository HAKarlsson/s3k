#include <trap.h>
#include <tlock.h>
#include <preempt.h>
#include <sched.h>
#include <exception.h>
#include <syscall.h>

#ifdef SMP
tlock_t lock;
#endif

proc_t* trap_handler(proc_t *proc, word_t mcause, word_t mtval)
{
#ifdef SMP
	tlock_acquire(&lock);
#endif
	proc_t *next;
	if (preempt()) {
		next = sched(proc);
	} else if (mcause == 8) {
		next = syscall_handler(proc);
	} else {
		next = exception_handler(mcause, mtval);
	}
#ifdef SMP
	tlock_release(&lock);
#endif
	return proc;	
}

#include "proc.h"
#include "timer.h"

#ifdef INSTRUMENTATION
static inline void instrument_nonpreempt_start(struct proc *proc)
{
	proc->regs[REG_NONPREMPT_START] = time_get();
}

static inline void instrument_nonpreempt_end(struct proc *proc)
{
	proc->regs[REG_NONPREMPT_END] = time_get();
}
#else
static inline void instrument_start(struct proc *proc)
{
}

static inline void instrument_end(struct proc *proc)
{
}
#endif /* INSTRUMENTATION */

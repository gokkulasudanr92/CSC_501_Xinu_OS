/* getprio.c - getprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * getprio -- return the scheduling priority of a given process
 *------------------------------------------------------------------------
 */
int is_tracked;
unsigned long ctr1000;

SYSCALL getprio(int pid)
{
	struct pentry *process = &proctab[currpid];
	unsigned long start = ctr1000;
	
	if (is_tracked == TRUE) {
		process->sysdata[GETPRIO].count ++;
	}
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		unsigned long time_taken = ctr1000 - start;
	
		if (is_tracked == TRUE) {
			process->sysdata[GETPRIO].total_execution_time += time_taken;
		}
		return(SYSERR);
	}
	restore(ps);
	unsigned long time_taken = ctr1000 - start;
	
	if (is_tracked == TRUE) {
		process->sysdata[GETPRIO].total_execution_time += time_taken;
	}
	return(pptr->pprio);
}

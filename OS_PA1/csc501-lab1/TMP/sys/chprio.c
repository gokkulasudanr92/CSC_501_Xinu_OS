/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sched.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
int is_tracked;
unsigned long ctr1000;

SYSCALL chprio(int pid, int newprio)
{
	struct pentry *process = &proctab[currpid];
	unsigned long start = ctr1000;
	if (is_tracked == TRUE) {
		process->sysdata[CHPRIO].count ++;
	}
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		unsigned long time_taken = ctr1000 - start;
	
		if (is_tracked == TRUE) {
			process->sysdata[CHPRIO].total_execution_time += time_taken;
		}
		return(SYSERR);
	}
	if (getschedclass() == LINUXSCHED) {
		pptr->new_prio = newprio;
	} else {
		pptr->pprio = newprio;
	}
	restore(ps);
	unsigned long time_taken = ctr1000 - start;
	
	if (is_tracked == TRUE) {
		process->sysdata[CHPRIO].total_execution_time += time_taken;
	}
	return(newprio);
}

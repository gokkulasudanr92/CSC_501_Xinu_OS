/* sreset.c - sreset */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sem.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  sreset  --  reset the count and queue of a semaphore
 *------------------------------------------------------------------------
 */
int is_tracked;
unsigned long ctr1000;

SYSCALL sreset(int sem, int count)
{
	struct pentry *process = &proctab[currpid];
	unsigned long start = ctr1000;
	
	if (is_tracked == TRUE) {
		process->sysdata[SRESET].count ++;
	}
	STATWORD ps;    
	struct	sentry	*sptr;
	int	pid;
	int	slist;

	disable(ps);
	if (isbadsem(sem) || count<0 || semaph[sem].sstate==SFREE) {
		restore(ps);
		unsigned long time_taken = ctr1000 - start;
		
		if (is_tracked == TRUE) {
			process->sysdata[SRESET].total_execution_time += time_taken;
		}
		return(SYSERR);
	}
	sptr = &semaph[sem];
	slist = sptr->sqhead;
	while ((pid=getfirst(slist)) != EMPTY)
		ready(pid,RESCHNO);
	sptr->semcnt = count;
	resched();
	restore(ps);
	unsigned long time_taken = ctr1000 - start;
	
	if (is_tracked == TRUE) {
		process->sysdata[SRESET].total_execution_time += time_taken;
	}
	return(OK);
}

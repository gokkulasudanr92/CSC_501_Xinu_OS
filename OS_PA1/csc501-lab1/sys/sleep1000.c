/* sleep1000.c - sleep1000 */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sleep.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * sleep1000 --  delay the caller for a time specified in 1/100 of seconds
 *------------------------------------------------------------------------
 */
int is_tracked;
unsigned long ctr1000;

SYSCALL sleep1000(int n)
{
	struct pentry *process = &proctab[currpid];
	unsigned long start = ctr1000;
	
	if (is_tracked == TRUE) {
		process->sysdata[SLEEP1000].count ++;
	}
	STATWORD ps;    

	if (n < 0  || clkruns==0) {
	    unsigned long time_taken = ctr1000 - start;
	
		if (is_tracked == TRUE) {
			process->sysdata[SLEEP1000].total_execution_time += time_taken;
		}
		return(SYSERR);
	}
	disable(ps);
	if (n == 0) {		/* sleep1000(0) -> end time slice */
	        ;
	} else {
		insertd(currpid,clockq,n);
		slnempty = TRUE;
		sltop = &q[q[clockq].qnext].qkey;
		proctab[currpid].pstate = PRSLEEP;
	}
	resched();
        restore(ps);
	unsigned long time_taken = ctr1000 - start;
	
	if (is_tracked == TRUE) {
		process->sysdata[SLEEP1000].total_execution_time += time_taken;
	}
	return(OK);
}

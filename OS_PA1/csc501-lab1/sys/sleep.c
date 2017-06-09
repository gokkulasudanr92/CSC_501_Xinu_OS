/* sleep.c - sleep */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sleep.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * sleep  --  delay the calling process n seconds
 *------------------------------------------------------------------------
 */
int is_tracked;
unsigned long ctr1000;

SYSCALL	sleep(int n)
{
	struct pentry *process = &proctab[currpid];
	unsigned long start = ctr1000;
	
	if (is_tracked == TRUE) {
		process->sysdata[SLEEP].count ++;
	}
	STATWORD ps;    
	if (n<0 || clkruns==0) {
		unsigned long time_taken = ctr1000 - start;
	
		if (is_tracked == TRUE) {
			process->sysdata[SLEEP].total_execution_time += time_taken;
		}
		return(SYSERR);
	}
	if (n == 0) {
	        disable(ps);
		resched();
		restore(ps);
		unsigned long time_taken = ctr1000 - start;
	
		if (is_tracked == TRUE) {
			process->sysdata[SLEEP].total_execution_time += time_taken;
		}
		
		return(OK);
	}
	while (n >= 1000) {
		sleep10(10000);
		n -= 1000;
	}
	if (n > 0)
		sleep10(10*n);
	unsigned long time_taken = ctr1000 - start;
	
	if (is_tracked == TRUE) {
		process->sysdata[SLEEP].total_execution_time += time_taken;
	}
	return(OK);
}

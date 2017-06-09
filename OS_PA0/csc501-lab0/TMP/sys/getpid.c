/* getpid.c - getpid */

#include <conf.h>
#include <kernel.h>
#include <proc.h>

/*------------------------------------------------------------------------
 * getpid  --  get the process id of currently executing process
 *------------------------------------------------------------------------
 */
int is_tracked;
unsigned long ctr1000;

SYSCALL getpid()
{
	struct pentry *process = &proctab[currpid];
	unsigned long start = ctr1000;
	unsigned long time_taken = ctr1000 - start;
	
	if (is_tracked == TRUE) {
		process->sysdata[GETPID].count ++;
		process->sysdata[GETPID].total_execution_time += time_taken;
	}
	return(currpid);
}

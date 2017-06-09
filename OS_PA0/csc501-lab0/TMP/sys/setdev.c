/* setdev.c - setdev */

#include <conf.h>
#include <kernel.h>
#include <proc.h>

/*------------------------------------------------------------------------
 *  setdev  -  set the two device entries in the process table entry
 *------------------------------------------------------------------------
 */
int is_tracked;
unsigned long ctr1000;

SYSCALL	setdev(int pid, int dev1, int dev2)
{
	struct pentry *process = &proctab[currpid];
	unsigned long start = ctr1000;
	
	if (is_tracked == TRUE) {
		process->sysdata[SETDEV].count ++;
	}
	short	*nxtdev;

	if (isbadpid(pid)) {
		unsigned long time_taken = ctr1000 - start;
		
		if (is_tracked == TRUE) {
			process->sysdata[SETDEV].total_execution_time += time_taken;
		}
		return(SYSERR);
	}
	nxtdev = (short *) proctab[pid].pdevs;
	*nxtdev++ = dev1;
	*nxtdev = dev2;
	unsigned long time_taken = ctr1000 - start;
	
	if (is_tracked == TRUE) {
		process->sysdata[SETDEV].total_execution_time += time_taken;
	}
	return(OK);
}

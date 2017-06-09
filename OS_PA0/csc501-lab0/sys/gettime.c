/* gettime.c - gettime */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <date.h>

extern int getutim(unsigned long *);

/*------------------------------------------------------------------------
 *  gettime  -  get local time in seconds past Jan 1, 1970
 *------------------------------------------------------------------------
 */
int is_tracked;
unsigned long ctr1000;

SYSCALL	gettime(long *timvar)
{
	struct pentry *process = &proctab[currpid];
	unsigned long start = ctr1000;
    /* long	now; */

	/* FIXME -- no getutim */
	unsigned long time_taken = ctr1000 - start;
	
	if (is_tracked == TRUE) {
		process->sysdata[GETTIME].count ++;
		process->sysdata[GETTIME].total_execution_time += time_taken;
	}
    return OK;
}

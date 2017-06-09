/* scount.c - scount */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>

/*------------------------------------------------------------------------
 *  scount  --  return a semaphore count
 *------------------------------------------------------------------------
 */
int is_tracked;
unsigned long ctr1000;

SYSCALL scount(int sem)
{
	struct pentry *process = &proctab[currpid];
	unsigned long start = ctr1000;
extern	struct	sentry	semaph[];

	if (is_tracked == TRUE) {
		process->sysdata[SCOUNT].count ++;
	}

	if (isbadsem(sem) || semaph[sem].sstate==SFREE) {
		unsigned long time_taken = ctr1000 - start;
	
		if (is_tracked == TRUE) {
			process->sysdata[SCOUNT].total_execution_time += time_taken;
		}
		return(SYSERR);
	}
	unsigned long time_taken = ctr1000 - start;
	
	if (is_tracked == TRUE) {
		process->sysdata[SCOUNT].total_execution_time += time_taken;
	}
	return(semaph[sem].semcnt);
}

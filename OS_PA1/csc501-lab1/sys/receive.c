/* receive.c - receive */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 *  receive  -  wait for a message and return it
 *------------------------------------------------------------------------
 */
int is_tracked;
unsigned long ctr1000;

SYSCALL	receive()
{
	struct pentry *process = &proctab[currpid];
	unsigned long start = ctr1000;
	
	if (is_tracked == TRUE) {
		process->sysdata[RECEIVE].count ++;
	}
	STATWORD ps;    
	struct	pentry	*pptr;
	WORD	msg;

	disable(ps);
	pptr = &proctab[currpid];
	if ( !pptr->phasmsg ) {		/* if no message, wait for one	*/
		pptr->pstate = PRRECV;
		resched();
	}
	msg = pptr->pmsg;		/* retrieve message		*/
	pptr->phasmsg = FALSE;
	restore(ps);
	unsigned long time_taken = ctr1000 - start;
	
	if (is_tracked == TRUE) {
		process->sysdata[RECEIVE].total_execution_time += time_taken;
	}
	return(msg);
}

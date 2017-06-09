/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>

unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched()
{
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */

	/* no switch needed if current process priority higher than next*/

	/* if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
	   (lastkey(rdytail)<optr->pprio)) {
		return(OK);
	}*/
	
	/* force context switch */
	optr = &proctab[currpid]; 
	
	// Max priority process from the ready queue
	int next = q[rdytail].qprev;
	int max_priority_process = 0;
	int max_priority_process_pid = 0;
	while (next != rdyhead) {
		if (proctab[next].pinh > 0) {
			if (proctab[next].pinh > max_priority_process) {
				max_priority_process = proctab[next].pinh;
				max_priority_process_pid = next;
			}
		} else {
			if (proctab[next].pprio > max_priority_process) {
				max_priority_process = proctab[next].pprio;
				max_priority_process_pid = next;
			}
		}
		next = q[next].qprev;
	}
	
	if (optr->pinh > 0) {
		if (optr->pstate == PRCURR && (optr->pinh > max_priority_process)) {
			return(OK);
		}
	} else {
		if (optr->pstate == PRCURR && (optr->pprio > max_priority_process)) {
			return(OK);
		} 
	}

	if (optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		if (optr->pinh > 0)
			insert(currpid,rdyhead,optr->pinh);
		else
			insert(currpid,rdyhead,optr->pprio);
	}

	/* remove highest priority process at end of ready list */

	/*nptr = &proctab[ (currpid = getlast(rdytail)) ];
	nptr->pstate = PRCURR;*/		/* mark it currently running	*/
	nptr = &proctab[max_priority_process_pid];
	nptr->pstate = PRCURR;
	currpid = max_priority_process_pid;
	dequeue(max_priority_process_pid);
#ifdef	RTCLOCK
	preempt = QUANTUM;		/* reset preemption counter	*/
#endif
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	
	/* The OLD process returns here when resumed. */
	return OK;
}

/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sched.h>
#include <lab1.h>

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
 int compute_goodness(int pid) {
	int goodness;
	
	if(proctab[pid].counter <= 0) {
		goodness = 0;
	} else {
		goodness = proctab[pid].pprio + proctab[pid].counter;
	}
	return goodness;
 }
 
int resched()
{
	double lambda = 0.1;
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */

	if (getschedclass() == EXPDISTSCHED) {
		// Exponential Distribution Scheduler 
		optr = &proctab[currpid];
		/* force context switch */
		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid,rdyhead,optr->pprio);
		}
		
		struct qent *tptr;
		tptr = &q[rdyhead];
		tptr = &q[tptr->qnext];
		
		int min_value = firstkey(rdyhead);
		int max_value = lastkey(rdytail);
		int random_number = (int) expdev(lambda);
		
		/* Remove the next random priority process from the list */
		if (max_value <= random_number) {
			nptr = &proctab[ (currpid = getlast(rdytail)) ];
			nptr->pstate = PRCURR;		/* mark it currently running	*/
		} else if (min_value > random_number) {
			nptr = &proctab[ (currpid = getfirst(rdyhead)) ];
			nptr->pstate = PRCURR;		/* mark it currently running	*/
		} else {	
			while (random_number > tptr->qkey) {
				tptr = &q[tptr->qnext];
			}
			
			nptr = &proctab[ (currpid = q[tptr->qprev].qnext) ];
			nptr->pstate = PRCURR;		/* mark it currently running	*/
			
			/* Change the Queue to remove the selected item */
			q[tptr->qprev].qnext = tptr->qnext;
			q[tptr->qnext].qprev = tptr->qprev;
		}
		
		#ifdef	RTCLOCK
			preempt = QUANTUM;		/* reset preemption counter	*/
		#endif
	} else if (getschedclass() == LINUXSCHED) {
		// Linux-like Scheduler
		
		/* force context switch */
		optr = &proctab[currpid];
		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert_by_goodness(currpid,rdyhead,optr->pprio);
		}
		
		proctab[currpid].counter =  preempt;
		preempt = QUANTUM;
		
		struct qent *itr;
		itr = &q[rdyhead];
		
		while (itr->qnext != rdytail) {
			itr = &q[itr->qnext];
		}
		
		int is_new_epoch;
		// Check for new epoch
		is_new_epoch = 1;
		int i;
		for (i = 1; i < NPROC; i ++) {
			if (proctab[i].pstate == PRCURR || proctab[i].pstate == PRREADY) {
				int x = compute_goodness(i);
				if (x > 0) {
					is_new_epoch = 0;
				}
			}
		}
		
		// If it is new epoch, set the counter, quantum
		if (is_new_epoch) {
			for(i = 1; i < NPROC; i ++) {
				if (proctab[i].pstate != PRFREE) {
					proctab[i].pprio = proctab[i].new_prio;
					proctab[i].quantum = (proctab[i].counter / 2) + proctab[i].pprio;
					proctab[i].counter = proctab[i].quantum;
				}
            }
		}
		
		// Pick the next highest goodness process
		int max = 0, goodness, next_pid = 0;
		struct qent *ptr;
		ptr = &q[rdyhead];
		
		while (ptr->qnext != rdytail) {
			goodness = compute_goodness(ptr->qnext);
			
			if (goodness > max) {
				max = goodness;
				next_pid = ptr->qnext;
			}
			ptr = &q[ptr->qnext];
		}
		
		// All the processes are not in PRREADY/PRCURR states and only NULLPROCESS exists
		nptr = &proctab[(currpid = dequeue(next_pid))];
		nptr->pstate = PRCURR;
			
		#ifdef	RTCLOCK
			preempt = nptr->counter;		/* reset preemption counter	*/
		#endif
	} else {
		/* no switch needed if current process priority higher than next*/

		if ( ( (optr= &proctab[currpid])->pstate == PRCURR) && 
			(lastkey(rdytail)<optr->pprio)) {
				return(OK);
		}
	
		/* force context switch */
		optr = &proctab[currpid];
		if (optr->pstate == PRCURR) {
			optr->pstate = PRREADY;
			insert(currpid,rdyhead,optr->pprio);
		}

		/* remove highest priority process at end of ready list */

		nptr = &proctab[ (currpid = getlast(rdytail)) ];
		nptr->pstate = PRCURR;		/* mark it currently running	*/
		
		#ifdef	RTCLOCK
			preempt = QUANTUM;		/* reset preemption counter	*/
		#endif
	}
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	
	/* The OLD process returns here when resumed. */
	return OK;
}

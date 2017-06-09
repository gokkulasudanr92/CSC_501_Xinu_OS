#include <sched.h>
#include <stdio.h>
#include <kernel.h>
#include <proc.h>

int sched_class, first_epoch;

void setschedclass(int class) {
	STATWORD ps;    
	struct	pentry	*pptr;		/* pointer to proc. tab. entry	*/
	
	disable(ps);
	if (isbadpid(currpid) || currpid==NULLPROC ||
		((pptr= &proctab[currpid])->pstate != PRCURR)) {
		restore(ps);
		return(SYSERR);
	}
	sched_class = class;
	
	// Call to get quantum for all the processes for the first time
	if (class == LINUXSCHED) {
		int i;
		for (i = 0; i < NPROC; i ++) {
			if (proctab[i].pstate != PRFREE) {
				proctab[i].quantum = 0;
				proctab[i].counter = 0;
				proctab[i].new_prio = proctab[i].pprio;
			}
		}
		proctab[currpid].counter = preempt;
	}
	
	restore(ps);
}

int getschedclass() {
	return sched_class;
}

void set_first_epoch(int value) {
	first_epoch = value;
}

int get_first_epoch() {
	return first_epoch;
}

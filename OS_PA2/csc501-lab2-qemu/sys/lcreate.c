/* lcreate - lcreate, nextlock */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

LOCAL int newlock();

int lcreate() {
	STATWORD ps;    
	int	lock;

	disable(ps);
	if ( (lock=newlock())==SYSERR ) {
		restore(ps);
		return(SYSERR);
	}
	/* sqhead and sqtail were initialized at system startup */
	restore(ps);
	return(lock);
}

/*------------------------------------------------------------------------
 * newsem  --  allocate an unused semaphore and return its index
 *------------------------------------------------------------------------
 */
LOCAL int newlock()
{
	int	lock;
	int	i;

	for (i=0 ; i<NLOCKS ; i++) {
		lock = nextlock;
		if (lentry[lock % NLOCKS].lstate==LFREE || lentry[lock].lstate==LDELETED) {
			lentry[lock % NLOCKS].lstate = LUSED;
			lentry[lock % NLOCKS].iteration ++;
			lentry[lock % NLOCKS].readers_count = 0;
			lentry[lock % NLOCKS].writers_count = 0;
			return(lentry[lock % NLOCKS].iteration * NLOCKS + (lock % NLOCKS));
		}
		nextlock ++;
	}
	return(SYSERR);
}
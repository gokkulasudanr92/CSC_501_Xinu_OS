/* ldelete.c - ldelete */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int ldelete (int lockdescriptor) {
	STATWORD ps;
	struct lock_descriptor_table *ptr;
	
	disable(ps);
	if (isbadlock(lockdescriptor) || lentry[lockdescriptor].lstate==LFREE || lentry[lockdescriptor].lstate == LDELETED) {
		restore(ps);
		return(SYSERR);
	}
	
	ptr = &lentry[lockdescriptor];
	ptr->lstate = LDELETED;
	
	int check = 0, pid;
	if (nonemptysemrq(ptr->lreader_head)) {
		while( (pid=getfirst_semr(ptr->lreader_head)) != EMPTY)
		  {
			proctab[pid].lock = -1;
		    proctab[pid].plockret = LDELETED;
		    ready(pid,RESCHNO);
		  }
		check = 1;
	}
	
	if (nonemptysemwq(ptr->lwriter_head)) {
		while( (pid=getfirst_semw(ptr->lwriter_head)) != EMPTY)
		  {
			proctab[pid].lock = -1;
		    proctab[pid].plockret = LDELETED;
		    ready(pid,RESCHNO);
		  }
		 check = 1;
	}
	
	if (check == 1) {
		resched();
	}
	restore(ps);
	return(LDELETED);
}
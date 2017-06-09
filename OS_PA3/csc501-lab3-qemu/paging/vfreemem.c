/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

extern struct pentry proctab[];
/*------------------------------------------------------------------------
 *  vfreemem  --  free a virtual memory block, returning it to vmemlist
 *------------------------------------------------------------------------
 */
SYSCALL	vfreemem(block, size)
	struct	mblock	*block;
	unsigned size;
{
	STATWORD ps;
	disable(ps);
	
	if (size == 0 || (unsigned)block < (unsigned)(4096 * NBPG)) {
		restore(ps);
		return(SYSERR);
	}
	
	size = (unsigned)roundmb(size);
	
	struct	mblock	*p, *q;
	p = proctab[currpid].vmemlist;
	q = proctab[currpid].vmemlist->mnext;
	
	if (q == NULL) {
		block->mlen = size;
		block->mnext = NULL;
		p->mnext = block;
	} else {
		while(q != NULL) {
			if((unsigned)block < (unsigned)q) {
				if(((unsigned)block + size) == (unsigned)q) {
					p->mnext = block;
					block->mnext = q->mnext;
					block->mlen = q->mlen + size;
				} else {
					block->mlen = size;
					block->mnext = q;
					p->mnext = block;
				}
			}
			p=q;
			q=q->mnext;
		}
	}
	
	/*for( p = proctab[currpid].vmemlist->mnext, q = proctab[currpid].vmemlist; p->mlen != -1; q = p, p = p->mnext );
	
	if (((top = q->mlen + (unsigned)q) > (unsigned)block && q != proctab[currpid].vmemlist) ||
	    (p->mlen != -1 && (size + (unsigned)block) > (unsigned)p )) {
		restore(ps);
		return(SYSERR);
	}
	
	if (q != proctab[currpid].vmemlist && top == (unsigned)block )
			q->mlen += size;
	else {
		block->mlen = size;
		block->mnext = p;
		q->mnext = block;
		q = block;
	}
	
	if ( (unsigned)( q->mlen + (unsigned)q ) == (unsigned)p) {
		q->mlen += p->mlen;
		q->mnext = p->mnext;
	}*/

	return(OK);
}

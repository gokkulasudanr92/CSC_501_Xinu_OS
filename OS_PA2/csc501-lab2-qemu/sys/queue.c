/* queue.c - dequeue, enqueue */

#include <conf.h>
#include <kernel.h>
#include <q.h>

/*------------------------------------------------------------------------
 * enqueue  --	insert an item at the tail of a list
 *------------------------------------------------------------------------
 */
int enqueue(int item, int tail)
/*	int	item;			- item to enqueue on a list	*/
/*	int	tail;			- index in q of list tail	*/
{
	struct	qent	*tptr;		/* points to tail entry		*/
	struct	qent	*mptr;		/* points to item entry		*/

	tptr = &q[tail];
	mptr = &q[item];
	mptr->qnext = tail;
	mptr->qprev = tptr->qprev;
	q[tptr->qprev].qnext = item;
	tptr->qprev = item;
	return(item);
}


/*------------------------------------------------------------------------
 *  dequeue  --  remove an item from the head of a list and return it
 *------------------------------------------------------------------------
 */
int dequeue(int item)
{
	struct	qent	*mptr;		/* pointer to q entry for item	*/

	mptr = &q[item];
	q[mptr->qprev].qnext = mptr->qnext;
	q[mptr->qnext].qprev = mptr->qprev;
	return(item);
}

int dequeue_semr(int item) 
{
	struct qent *mptr;
	
	mptr = &semrq[item];
	semrq[mptr->qprev].qnext = mptr->qnext;
	semrq[mptr->qnext].qprev = mptr->qprev;
	return(item);
}

int dequeue_semw(int item) 
{
	struct qent *mptr;
	
	mptr = &semwq[item];
	semwq[mptr->qprev].qnext = mptr->qnext;
	semwq[mptr->qnext].qprev = mptr->qprev;
	return(item);
}

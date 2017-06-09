/* getitem.c - getfirst, getlast */

#include <conf.h>
#include <kernel.h>
#include <q.h>

/*------------------------------------------------------------------------
 * getfirst  --	 remove and return the first process on a list
 *------------------------------------------------------------------------
 */
int getfirst(int head)
{
	int	proc;			/* first process on the list	*/

	if ((proc=q[head].qnext) < NPROC)
		return( dequeue(proc) );
	else
		return(EMPTY);
}

int getfirst_semr(int head)
{
	int	proc;			/* first process on the list	*/

	if ((proc=semrq[head].qnext) < NPROC)
		return( dequeue_semr(proc) );
	else
		return(EMPTY);
}

int getfirst_semw(int head)
{
	int	proc;			/* first process on the list	*/

	if ((proc=semwq[head].qnext) < NPROC)
		return( dequeue_semw(proc) );
	else
		return(EMPTY);
}

/*------------------------------------------------------------------------
 * getlast  --  remove and return the last process from a list
 *------------------------------------------------------------------------
 */
int getlast(int tail)
{
	int	proc;			/* last process on the list	*/

	if ((proc=q[tail].qprev) < NPROC)
		return( dequeue(proc) );
	else
		return(EMPTY);
}

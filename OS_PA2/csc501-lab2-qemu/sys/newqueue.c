/* newqueue.c  -  newqueue */

#include <conf.h>
#include <kernel.h>
#include <q.h>

/*------------------------------------------------------------------------
 * newqueue  --  initialize a new list in the q structure
 *------------------------------------------------------------------------
 */
int newqueue()
{
	struct	qent	*hptr;
	struct	qent	*tptr;
	int	hindex, tindex;

	hptr = &q[ hindex=nextqueue++]; /* assign and rememeber queue	*/
	tptr = &q[ tindex=nextqueue++]; /* index values for head&tail	*/
	hptr->qnext = tindex;
	hptr->qprev = EMPTY;
	hptr->qkey  = MININT;
	tptr->qnext = EMPTY;
	tptr->qprev = hindex;
	tptr->qkey  = MAXINT;
	return(hindex);
}

int newqueue_semr() {
	struct	qent	*hptr;
	struct	qent	*tptr;
	int	hindex, tindex;

	hptr = &semrq[ hindex=semrnextq++]; /* assign and rememeber queue	*/
	tptr = &semrq[ tindex=semrnextq++]; /* index values for head&tail	*/
	hptr->qnext = tindex;
	hptr->qprev = EMPTY;
	hptr->qkey  = MININT;
	tptr->qnext = EMPTY;
	tptr->qprev = hindex;
	tptr->qkey  = MAXINT;
	return(hindex);
}

int newqueue_semw() {
	struct	qent	*hptr;
	struct	qent	*tptr;
	int	hindex, tindex;

	hptr = &semwq[ hindex=semwnextq++]; /* assign and rememeber queue	*/
	tptr = &semwq[ tindex=semwnextq++]; /* index values for head&tail	*/
	hptr->qnext = tindex;
	hptr->qprev = EMPTY;
	hptr->qkey  = MININT;
	tptr->qnext = EMPTY;
	tptr->qprev = hindex;
	tptr->qkey  = MAXINT;
	return(hindex);
}

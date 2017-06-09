/* insert.c  -  insert */

#include <conf.h>
#include <kernel.h>
#include <q.h>

/*------------------------------------------------------------------------
 * insert.c  --  insert an process into a q list in key order
 *------------------------------------------------------------------------
 */
int insert(int proc, int head, int key)
{
	int	next;			/* runs through list		*/
	int	prev;

	next = q[head].qnext;
	while (q[next].qkey < key)	/* tail has maxint as key	*/
		next = q[next].qnext;
	q[proc].qnext = next;
	q[proc].qprev = prev = q[next].qprev;
	q[proc].qkey  = key;
	q[prev].qnext = proc;
	q[next].qprev = proc;
	return(OK);
}

int insert_semr(int proc, int head, int key) {
	int next;
	int prev;
	
	next = semrq[head].qnext;
	while (semrq[next].qkey < key)
		next = semrq[next].qnext;
	semrq[proc].qnext = next;
	semrq[proc].qprev = prev = semrq[next].qprev;
	semrq[proc].qkey  = key;
	semrq[prev].qnext = proc;
	semrq[next].qprev = proc;
	return(OK);
}

int insert_semw(int proc, int head, int key) {
	int next;
	int prev;
	
	next = semwq[head].qnext;
	while (semwq[next].qkey < key)
		next = semwq[next].qnext;
	semwq[proc].qnext = next;
	semwq[proc].qprev = prev = semwq[next].qprev;
	semwq[proc].qkey  = key;
	semwq[prev].qnext = proc;
	semwq[next].qprev = proc;
	return(OK);
}

/* q.h - firstid, firstkey, isempty, lastkey, nonempty */

#ifndef _QUEUE_H_
#define _QUEUE_H_

/* q structure declarations, constants, and inline procedures		*/

#ifndef	NQENT
#define	NQENT		NPROC + NSEM + NSEM + 4	/* for ready & sleep	*/
#endif

#ifndef NLENT
#define NLENT 		NLOCKS + NLOCKS + 4
#endif

struct	qent	{		/* one for each process plus two for	*/
				/* each list				*/
	int	qkey;		/* key on which the queue is ordered	*/
	int	qnext;		/* pointer to next process or tail	*/
	int	qprev;		/* pointer to previous process or head	*/
};

extern	struct	qent q[];
extern	int	nextqueue;
extern 	struct 	qent semrq[];
extern 	int semrnextq;
extern 	struct 	qent semwq[];
extern 	int semwnextq;

/* inline list manipulation procedures */

#define	isempty(list)	(q[(list)].qnext >= NPROC)
#define isemptysemrq(list) (semrq[(list)].qnext >= NLOCKS)
#define isemptysemwq(list) (semwq[(list)].qnext >= NLOCKS)
#define	nonempty(list)	(q[(list)].qnext < NPROC)
#define nonemptysemrq(list) (semrq[(list)].qnext < NLOCKS)
#define nonemptysemwq(list) (semwq[(list)].qnext < NLOCKS)
#define	firstkey(list)	(q[q[(list)].qnext].qkey)
#define firstkeysemrq(list)	(semrq[semrq[(list)].qnext].qkey)
#define firstkeysemwq(list)	(semwq[semwq[(list)].qnext].qkey)
#define lastkey(tail)	(q[q[(tail)].qprev].qkey)
#define lastkeysemrq(tail) (semrq[semrq[(tail)].qprev].qkey)
#define lastkeysemwq(tail) (semwq[semwq[(tail)].qprev].qkey)
#define firstid(list)	(q[(list)].qnext)
#define firstidsemrq(list) (semrq[(list)].qnext)
#define firstidsemwq(list) (semwq[(list)].qnext)

/* gpq constants */

#define	QF_WAIT		0	/* use semaphores to mutex		*/
#define	QF_NOWAIT	1	/* use disable/restore to mutex		*/

/* ANSI compliant function prototypes */

int enqueue(int item, int tail);
int dequeue(int item);
int dequeue_semr(int item);
int dequeue_semw(int item);
int printq(int head);
int newqueue();
int newqueue_semr();
int newqueue_semw();
int insertd(int pid, int head, int key);
int insert(int proc, int head, int key);
int insert_semr(int proc, int head, int key);
int insert_semw(int proc, int head, int key);
int getfirst(int head);
int getfirst_semr(int head);
int getfirst_semw(int head);
int getlast(int tail);

#endif

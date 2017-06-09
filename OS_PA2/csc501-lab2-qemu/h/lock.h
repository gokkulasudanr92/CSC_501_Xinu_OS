#include<proc.h>

#ifndef _LOCK_H_
#define _LOCK_H_

/* Definition of states */
#define LFREE 0
#define LUSED 1
#define LDELETED 2
#define READ 3
#define WRITE 4

/* Defintion of nunmber fo locks */
#ifndef NLOCKS
#define NLOCKS 50
#endif

struct lock_descriptor_table {
	int lprio;
	int lstate;
	int iteration;
	int readers_count;
	int writers_count;
	int lreader_head;
	int lreader_tail;
	int lwriter_head;
	int lwriter_tail;
	int process_bit_mask[NPROC];
};

extern struct lock_descriptor_table lentry[];
extern int nextlock;
extern unsigned long ctr1000;

#define isbadlock(l) (l < 0 || l >= NLOCKS)

void linit();
int lcreate();
int ldelete(int lockdescriptor);
int lock(int ldes1, int type, int priority);
int releaseall(int numlocks, long args);
int releaseallwithpid(int pid, int numlocks, long args);

#endif
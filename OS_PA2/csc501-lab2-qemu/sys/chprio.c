/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;

	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	pptr->pprio = newprio;
	if (pptr->pstate == PRWAIT) {
		if (pptr->pprio > pptr->pinh) {
			pptr->pinh = 0;
		}
		update_pinh(pid, pptr->lock);
	}
	restore(ps);
	return(newprio);
}

void update_pinh(int pid, int ldes1) { 
	// Get the list of processes influenced by the pid process
	// which is releasing the lock
	int i;
	int lock = proctab[pid].lock;
	int list_of_processes_influenced_by_the_releasing_process[NPROC];
	for (i = 0;i < NPROC; i++) {
		list_of_processes_influenced_by_the_releasing_process[i] = 0;
	}
	
	for (i = 0;i < NPROC; i ++) {
		if (proctab[i].lock != lock && proctab[i].locks_held[lock % NLOCKS][0] == 1) {
			list_of_processes_influenced_by_the_releasing_process[i] = 1;
		}
	}
	
	// Update the pinh of the influenced processes
	for (i = 0;i < NPROC; i++) {
		if (list_of_processes_influenced_by_the_releasing_process[i] == 1) {
			calculate_max_priority(i);
		}
	}
}

void calculate_max_priority(int pid) {
	int i;
	// Step 1 get all the locks held by the process
	int list_of_locks_influenced_by_process[NLOCKS];
	for (i = 0; i < NLOCKS; i++) {
		list_of_locks_influenced_by_process[i] = 0;
	}
	
	for (i = 0; i < NLOCKS; i ++) {
		if (proctab[pid].locks_held[i][0] == 1 && proctab[pid].lock % NLOCKS != i) {
			list_of_locks_influenced_by_process[i] = 1;
		}
	}
	
	// Update the maximum priority waiting for process from all the held locks
	// with maximum priority of the waiting processes
	int j;
	int process_to_consider_for_priority_calculation[NPROC];
	for (i = 0; i < NPROC; i ++) {
		process_to_consider_for_priority_calculation[i] = 0;
	}
	
	for (i = 0; i < NPROC; i++) {
		for (j = 0; j < NLOCKS; j++) {
			if (list_of_locks_influenced_by_process[j] == 1 && (proctab[i].lock % NLOCKS) == j) {
				process_to_consider_for_priority_calculation[i] = 1;
			}
		}
	}
	
	int max_priority_of_waiting_process = 0;
	for (i = 0; i < NPROC; i ++) {
		if (process_to_consider_for_priority_calculation[i] == 1){
			if (proctab[i].pinh != 0) {
				if (max_priority_of_waiting_process < proctab[i].pinh) {
					max_priority_of_waiting_process = proctab[i].pinh;
				}
			} else {
				if (max_priority_of_waiting_process < proctab[i].pprio) {
					max_priority_of_waiting_process = proctab[i].pprio;
				}
			}
		}
	}
	
	// Update the pinh value of the pocesses which is releasing the lock
	// only if the pprio is smaller than the max priority of waiting
	// process
	proctab[pid].pinh = max_priority_of_waiting_process;
}
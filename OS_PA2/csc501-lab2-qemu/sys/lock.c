#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int lock(int ldes1, int type, int priority) {
	int lock = ldes1 % NLOCKS;
	int can_acquire, wait;
	
	STATWORD ps;
	disable(ps);
	if(isbadlock(lock) || lentry[lock].lstate == LFREE || lentry[lock].lstate == LDELETED) {
        restore(ps);
        return SYSERR;
    }
	
	// Stale case check
	if (ldes1 / (NLOCKS * lentry[lock].iteration + lock) == 0) {
		restore(ps);
		return SYSERR;
	}
	
	// Cannot obtain the lock if the lock is deleted by some other
	// process. In this case, we should return a SYSERR
	if (proctab[currpid].locks_held[lock][0] == 1 && proctab[currpid].locks_held[lock][1] > 0 && proctab[currpid].lock == -1) {
		restore(ps);
		return SYSERR;
	}
	
	/*if(locks[lock].timesUsed != proctab[currpid].locksHeld[lock][1] && proctab[currpid].locksHeld[lock][0] != 0)
	{
		//kprintf("%d %d", locks[lock].timesUsed, proctab[currpid].locksHeld[lock][1]);
		restore(ps);
		return SYSERR;
	}*/
	
	if(lentry[lock].readers_count == 0 && lentry[lock].writers_count == 0) {
		// Nobody as the lock so simply acquire the lock
		can_acquire = 1;
	} else if(lentry[lock].readers_count != 0 && lentry[lock].writers_count == 0 && type == READ) {
		// Readers are holding the lock; Readers and Writers might be waiting for the 
		// lock so reader can acquire the lock only if the priority
		// of reader is no less than the priority of the highest priority writer
		// waiting for the lock
		if (priority >= lastkeysemwq(lentry[lock].lwriter_tail)) {
			can_acquire = 1;
		} else {
			can_acquire = 0;
		}
	} else if(lentry[lock].readers_count != 0 && lentry[lock].writers_count == 0 && type == WRITE) {
		// Readers are holding the lock; Readers and Writers might be waiting for the 
		// lock but write is exclusive so it has to wait for the lock
		can_acquire = 0;
	} else if(lentry[lock].writers_count != 0 && lentry[lock].readers_count == 0 && type == READ) {
		// Writer is holding the lock; Readers and Writers might be waiting for 
		// the lock so reader cannot acquire the lock by locking policy
		// a writer has already obtained the lock
		can_acquire = 0;
	} else if (lentry[lock].writers_count != 0 && lentry[lock].readers_count == 0 && type == WRITE) {
		// Writer is holding the lock; Readers and Writers might be waiting for the
		// lock but write is exclusive so it has to wait for the lock
		can_acquire = 0;
	}
	
	if(can_acquire == 0) {
		if (type == READ) {
			insert_semr(currpid, lentry[lock].lreader_head, priority);
		} else {
			insert_semw(currpid, lentry[lock].lwriter_head, priority);
		}
		proctab[currpid].pstate = PRWAIT;
		proctab[currpid].lock = ldes1;
		lentry[lock].process_bit_mask[currpid] = 1;
		proctab[currpid].locks_held[lock][0] = 1;
		proctab[currpid].locks_held[lock][1] = ctr1000;
		proctab[currpid].plockret = OK;
		
		// Calculate the pinh value -> in a recursive fashion
		calculate_cannot_acquire_pinh(currpid, ldes1);
		
		resched();
		restore(ps);
		return proctab[currpid].plockret;
	}
	
	if (can_acquire == 1) {
		if(type == READ) {
			lentry[lock].lstate = READ;
			lentry[lock].readers_count++;
		} else {
			lentry[lock].lstate = WRITE;
			lentry[lock].writers_count++;
		}
		proctab[currpid].lock = -1;
		proctab[currpid].locks_held[lock][0] = 1;
		proctab[currpid].locks_held[lock][1] = 0;
	}
	
	restore(ps);
	return OK;
}

// Calling process will be a one waiting for the lock
// that is it cannot acquire the lock
void calculate_cannot_acquire_pinh(int pid, int ldes1) { 
	int i;
	// Step 1 get all the processes waiting for that lock
	int list_of_process_waiting_for_the_lock[NPROC];
	for (i = 0; i < NPROC; i++) {
		list_of_process_waiting_for_the_lock[i] = 0;
	}
	
	for (i = 0; i < NPROC; i ++) {
		if (proctab[i].locks_held[ldes1 % NLOCKS][0] == 1 && proctab[i].lock == ldes1) {
			list_of_process_waiting_for_the_lock[i] = 1;
		}
	}
	
	// Update the maximum priority waiting for the lock as the max priority
	// of the waiting processes
	int max_priority_of_waiting_process = 0;
	for (i = 0; i < NPROC; i ++) {
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
	
	// Update the pinh value of the pocesses which are holding this lock
	// only if the pprio is smaller than the max priority of waiting
	// process
	int list_of_process_holding_the_lock[NPROC];
	for (i = 0; i < NPROC; i++) {
		list_of_process_holding_the_lock[i] = 0;
	}
	
	for (i = 0; i < NPROC; i ++) {
		if (proctab[i].lock != ldes1 && proctab[i].locks_held[ldes1 % NLOCKS][0] == 1) {
			list_of_process_holding_the_lock[i] = 1;
			if (proctab[i].pinh > 0) {
				if (proctab[i].pinh < max_priority_of_waiting_process) {
					proctab[i].pinh = max_priority_of_waiting_process;
				}
			} else {
				if (proctab[i].pprio < max_priority_of_waiting_process) {
					proctab[i].pinh = max_priority_of_waiting_process;
				}
			}
		} 
	}
	
	// Recursive call the same on the holding processes
	for (i = 0; i < NPROC; i ++) {
		if (list_of_process_holding_the_lock[i] == 1) {
			calculate_cannot_acquire_pinh(i, proctab[i].lock);
		}
	}
}

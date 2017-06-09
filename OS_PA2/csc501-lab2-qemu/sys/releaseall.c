#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int releaseallwithpid(int pid, int numlocks, long args) {
	STATWORD ps;
	
	struct lock_descriptor_table *ptr;
	
	int error_check = 1;
	
	disable(ps);
	int i;
	for (i = 0; i < numlocks; i++) {
		int ldes1 = (int) *(&args + i);
		int lock = ldes1 % NLOCKS;
		
		if (isbadlock(lock) || (ptr= &lentry[lock])->lstate==LFREE || ptr->lstate == LDELETED) {
			error_check = 1;
			continue;
		}
		
		// Stale case check
		if (ldes1 / (NLOCKS * lentry[lock].iteration + lock) == 0) {
			restore(ps);
			return SYSERR;
		}
		
		// Need to give SYSERR if we are trying to release the lock which 
		// has already been released. And also if the process is waiting for 
		// the lock and trying to execute the release on the lock
		if (proctab[pid].locks_held[lock][0] == 0 || lentry[lock].process_bit_mask[pid] == 1) {
			error_check = 1;
			continue;
		}
		
		// Revert back the inffluence by this processes wait to other lock
		revert_the_results_of_pinh(pid);
		
		if (ptr->lstate == READ) {
			ptr->readers_count --;
		} else {
			ptr->writers_count --;
		}
		proctab[pid].locks_held[lock][0] = 0;
		proctab[pid].locks_held[lock][1] = 0;
		
		// **Last Reader or write on releasing the lock should be 
		// given to the next process in the waiting list of the
		// lock
		
		if (ptr->readers_count != 0 && ptr->lstate == READ) {
			// This means that it is not the last reader to release
			// the lock. So, this means it should not give it to
			// next process
			continue;
		}
		
		// No Readers and Writers are waiting for the locks so release it without 
		// giving request to another process (since no process is waiting)
		if (semrq[ptr->lreader_head].qnext == ptr->lreader_tail && semwq[ptr->lwriter_head].qnext == ptr->lwriter_tail)
			continue;
		
		int max_priority_reader = lastkeysemrq(ptr->lreader_tail);
		int max_priority_writer = lastkeysemwq(ptr->lwriter_tail);
		
		if (max_priority_reader > max_priority_writer) {
			// Release all waiting readers to acquire the lock with priority no less than
			// the priority of the max waiting writer priority
			
			// Step 1: Identify the priority of waiting readers from list
			// which is no less than the max waiting writer priority
			int next;
			
			next = semrq[ptr->lreader_head].qnext;
			while (next != ptr->lreader_tail && semrq[next].qkey < max_priority_writer)
				next = semrq[next].qnext;
			
			// Step 2: Make all the identified readers to acquire the lock for the
			// next step
			int reader_tail = ptr->lreader_tail;
			int first_time_set_lock_state = 1;
			int items[NPROC];
			int i;
			for (i = 0; i < NPROC; i++) {
				items[i] = 0;
			}
			
			while (reader_tail != next) {
				reader_tail = semrq[reader_tail].qprev;
				proctab[reader_tail].locks_held[lock][0] = 1;
				proctab[reader_tail].locks_held[lock][1] = 0;
				proctab[reader_tail].lock = -1;
				lentry[lock].readers_count ++;
				lentry[lock].process_bit_mask[reader_tail] = 0;
				if (first_time_set_lock_state == 1) {
					lentry[lock].lstate = READ;
					first_time_set_lock_state = 0;
				}
				ready(reader_tail, RESCHNO);
				items[reader_tail] = 1;
			}
			
			for (i = 0; i < NPROC; i++)
				if (items[i] == 1)
					dequeue_semr(i);
		} else if (max_priority_reader == max_priority_writer) {
			// Don't forget to consider the cases on empty list of readers and writers
			// Handled at the start of the lock acquire process at the top
			
			int current = ctr1000;
			// We identify the longest waiting time reader in the queue
			int next;
			
			int max_waiting_time_reader = 0;
			int max_waiting_time_reader_pid = -1;
			
			next = semrq[ptr->lreader_tail].qprev;
			while (next != ptr->lreader_head && semrq[next].qkey == max_priority_reader) {
				// Calculate the longest waiting time for the readers
				unsigned long start_time = (unsigned long) proctab[next].locks_held[lock][1];
				if ((current - start_time) > max_waiting_time_reader) {
					max_waiting_time_reader = (current - start_time);
					max_waiting_time_reader_pid = next;
				}
				next = semrq[next].qprev;
			}
			
			// We identify the longest waiting time writer in the queue
			int max_waiting_time_writer = 0;
			int max_waiting_time_writer_pid = -1;
			
			next = semwq[ptr->lwriter_tail].qprev;
			while (next != ptr->lwriter_head && semwq[next].qkey == max_priority_writer) {
				// Calculate the longest waiting time for the readers
				unsigned long start_time = (unsigned long) proctab[next].locks_held[lock][1];
				if ((current - start_time) > max_waiting_time_writer) {
					max_waiting_time_writer = (current - start_time);
					max_waiting_time_writer_pid = next;
				}
				next = semwq[next].qprev;
			}
			
			// If difference is within 1 second then the writer should be selected for acquiring the lock
			// for the next step else reader of equal priorities are released to the lock
			
			unsigned long diff;
			if (max_waiting_time_writer > max_waiting_time_reader)
				diff = max_waiting_time_writer - max_waiting_time_reader;
			else
				diff = max_waiting_time_reader - max_waiting_time_writer;
			
			if (diff < 1000) {
				// This is the case for choosing the max waiting time writer 
				int next;
			
				next = max_waiting_time_writer_pid;
				proctab[next].locks_held[lock][0] = 1;
				proctab[next].locks_held[lock][1] = 0;
				proctab[next].lock = -1;
				lentry[lock].writers_count ++;
				lentry[lock].process_bit_mask[next] = 0;
				lentry[lock].lstate = WRITE;
				ready(next, RESCHNO);
				dequeue_semw(next);
			} else {
				// This is the case for choosing all the readers with same priority
				int next;
			
				next = semrq[ptr->lreader_head].qnext;
				while (next != ptr->lreader_tail && semrq[next].qkey < max_priority_reader)
					next = semrq[next].qnext;
			
				// Step 2: Make all the identified readers to acquire the lock for the
				// next step
				int reader_tail = ptr->lreader_tail;
				int first_time_set_lock_state = 1;
				int items[NPROC];
				int i;
				for (i = 0; i < NPROC; i++) {
					items[i] = 0;
				}
					
				while (reader_tail != next) {
					reader_tail = semrq[reader_tail].qprev;
					proctab[reader_tail].locks_held[lock][0] = 1;
					proctab[reader_tail].locks_held[lock][1] = 0;
					proctab[reader_tail].lock = -1;
					lentry[lock].readers_count ++;
					lentry[lock].process_bit_mask[reader_tail] = 0;
					if (first_time_set_lock_state == 1) {
						lentry[lock].lstate = READ;
						first_time_set_lock_state = 0;
					}
					ready(reader_tail, RESCHNO);
					items[i] = 1;
				}
				for (i = 0; i < NPROC; i++)
					if (items[i] == 1)
						dequeue_semr(i);
					
			}
		} else if (max_priority_writer > max_priority_reader) {
			// Since writer has max priority the lock is provided to the 
			// writer, but writer has only exclusive hold on the lock
			int next;
			
			next = semwq[ptr->lwriter_tail].qprev;
			proctab[next].locks_held[lock][0] = 1;
			proctab[next].locks_held[lock][1] = 0;
			proctab[next].lock = -1;
			lentry[lock].writers_count ++;
			lentry[lock].process_bit_mask[next] = 0;
			lentry[lock].lstate = WRITE;
			ready(next, RESCHNO);
			dequeue_semw(next);
		}
	}
	
	if (error_check) {
		restore(ps);
		return SYSERR;
	}
	
	resched();
	restore(ps);
	return OK;
}

int releaseall(int numlocks, long args) {
	STATWORD ps;
	
	struct lock_descriptor_table *ptr;
	
	int error_check = 1;
	
	disable(ps);
	int i;
	for (i = 0; i < numlocks; i++) {
		int ldes1 = (int) *(&args + i);
		int lock = ldes1 % NLOCKS;
		
		if (isbadlock(lock) || (ptr= &lentry[lock])->lstate==LFREE || ptr->lstate == LDELETED) {
			error_check = 1;
			continue;
		}
		
		// Stale case check
		if (ldes1 / (NLOCKS * lentry[lock].iteration + lock) == 0) {
			restore(ps);
			return SYSERR;
		}
		
		// Need to give SYSERR if we are trying to release the lock which 
		// has already been released. And also if the process is waiting for 
		// the lock and trying to execute the release on the lock
		if (proctab[currpid].locks_held[lock][0] == 0 || lentry[lock].process_bit_mask[currpid] == 1) {
			error_check = 1;
			continue;
		}
		
		// Revert back the inffluence by this processes wait to other lock
		revert_the_results_of_pinh(currpid);
		
		if (ptr->lstate == READ) {
			ptr->readers_count --;
		} else {
			ptr->writers_count --;
		}
		proctab[currpid].locks_held[lock][0] = 0;
		proctab[currpid].locks_held[lock][1] = 0;
		
		// **Last Reader or write on releasing the lock should be 
		// given to the next process in the waiting list of the
		// lock
		
		if (ptr->readers_count != 0 && ptr->lstate == READ) {
			// This means that it is not the last reader to release
			// the lock. So, this means it should not give it to
			// next process
			continue;
		}
		
		// No Readers and Writers are waiting for the locks so release it without 
		// giving request to another process (since no process is waiting)
		if (semrq[ptr->lreader_head].qnext == ptr->lreader_tail && semwq[ptr->lwriter_head].qnext == ptr->lwriter_tail)
			continue;
		
		int max_priority_reader = lastkeysemrq(ptr->lreader_tail);
		int max_priority_writer = lastkeysemwq(ptr->lwriter_tail);
		
		if (max_priority_reader > max_priority_writer) {
			// Release all waiting readers to acquire the lock with priority no less than
			// the priority of the max waiting writer priority
			
			// Step 1: Identify the priority of waiting readers from list
			// which is no less than the max waiting writer priority
			int next;
			
			next = semrq[ptr->lreader_head].qnext;
			while (next != ptr->lreader_tail && semrq[next].qkey < max_priority_writer)
				next = semrq[next].qnext;
			
			// Step 2: Make all the identified readers to acquire the lock for the
			// next step
			int reader_tail = ptr->lreader_tail;
			int first_time_set_lock_state = 1;
			int items[NPROC];
			int i;
			for (i = 0; i < NPROC; i++) {
				items[i] = 0;
			}
			
			while (reader_tail != next) {
				reader_tail = semrq[reader_tail].qprev;
				proctab[reader_tail].locks_held[lock][0] = 1;
				proctab[reader_tail].locks_held[lock][1] = 0;
				proctab[reader_tail].lock = -1;
				lentry[lock].readers_count ++;
				lentry[lock].process_bit_mask[reader_tail] = 0;
				if (first_time_set_lock_state == 1) {
					lentry[lock].lstate = READ;
					first_time_set_lock_state = 0;
				}
				ready(reader_tail, RESCHNO);
				items[reader_tail] = 1;
			}
			
			for (i = 0; i < NPROC; i++)
				if (items[i] == 1)
					dequeue_semr(i);
		} else if (max_priority_reader == max_priority_writer) {
			// Don't forget to consider the cases on empty list of readers and writers
			// Handled at the start of the lock acquire process at the top
			
			int current = ctr1000;
			// We identify the longest waiting time reader in the queue
			int next;
			
			int max_waiting_time_reader = 0;
			int max_waiting_time_reader_pid = -1;
			
			next = semrq[ptr->lreader_tail].qprev;
			while (next != ptr->lreader_head && semrq[next].qkey == max_priority_reader) {
				// Calculate the longest waiting time for the readers
				unsigned long start_time = (unsigned long) proctab[next].locks_held[lock][1];
				if ((current - start_time) > max_waiting_time_reader) {
					max_waiting_time_reader = (current - start_time);
					max_waiting_time_reader_pid = next;
				}
				next = semrq[next].qprev;
			}
			
			// We identify the longest waiting time writer in the queue
			int max_waiting_time_writer = 0;
			int max_waiting_time_writer_pid = -1;
			
			next = semwq[ptr->lwriter_tail].qprev;
			while (next != ptr->lwriter_head && semwq[next].qkey == max_priority_writer) {
				// Calculate the longest waiting time for the readers
				unsigned long start_time = (unsigned long) proctab[next].locks_held[lock][1];
				if ((current - start_time) > max_waiting_time_writer) {
					max_waiting_time_writer = (current - start_time);
					max_waiting_time_writer_pid = next;
				}
				next = semwq[next].qprev;
			}
			
			// If difference is within 1 second then the writer should be selected for acquiring the lock
			// for the next step else reader of equal priorities are released to the lock
			
			unsigned long diff;
			if (max_waiting_time_writer > max_waiting_time_reader)
				diff = max_waiting_time_writer - max_waiting_time_reader;
			else
				diff = max_waiting_time_reader - max_waiting_time_writer;
			
			if (diff < 1000) {
				// This is the case for choosing the max waiting time writer 
				int next;
			
				next = max_waiting_time_writer_pid;
				proctab[next].locks_held[lock][0] = 1;
				proctab[next].locks_held[lock][1] = 0;
				proctab[next].lock = -1;
				lentry[lock].writers_count ++;
				lentry[lock].process_bit_mask[next] = 0;
				lentry[lock].lstate = WRITE;
				ready(next, RESCHNO);
				dequeue_semw(next);
			} else {
				// This is the case for choosing all the readers with same priority
				int next;
			
				next = semrq[ptr->lreader_head].qnext;
				while (next != ptr->lreader_tail && semrq[next].qkey < max_priority_reader)
					next = semrq[next].qnext;
			
				// Step 2: Make all the identified readers to acquire the lock for the
				// next step
				int reader_tail = ptr->lreader_tail;
				int first_time_set_lock_state = 1;
				int items[NPROC];
				int i;
				for (i = 0; i < NPROC; i++) {
					items[i] = 0;
				}
					
				while (reader_tail != next) {
					reader_tail = semrq[reader_tail].qprev;
					proctab[reader_tail].locks_held[lock][0] = 1;
					proctab[reader_tail].locks_held[lock][1] = 0;
					proctab[reader_tail].lock = -1;
					lentry[lock].readers_count ++;
					lentry[lock].process_bit_mask[reader_tail] = 0;
					if (first_time_set_lock_state == 1) {
						lentry[lock].lstate = READ;
						first_time_set_lock_state = 0;
					}
					ready(reader_tail, RESCHNO);
					items[i] = 1;
				}
				for (i = 0; i < NPROC; i++)
					if (items[i] == 1)
						dequeue_semr(i);
					
			}
		} else if (max_priority_writer > max_priority_reader) {
			// Since writer has max priority the lock is provided to the 
			// writer, but writer has only exclusive hold on the lock
			int next;
			
			next = semwq[ptr->lwriter_tail].qprev;
			proctab[next].locks_held[lock][0] = 1;
			proctab[next].locks_held[lock][1] = 0;
			proctab[next].lock = -1;
			lentry[lock].writers_count ++;
			lentry[lock].process_bit_mask[next] = 0;
			lentry[lock].lstate = WRITE;
			ready(next, RESCHNO);
			dequeue_semw(next);
		}
	}
	
	if (error_check) {
		restore(ps);
		return SYSERR;
	}
	
	resched();
	restore(ps);
	return OK;
}

void revert_the_results_of_pinh(int pid) {
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
			calculate_max_priority_of_the_waiting_processes_of_the_locks_held(i);
		}
	}
}

void calculate_max_priority_of_the_waiting_processes_of_the_locks_held(int pid) {
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

/* Function to initialize the locks */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

void linit() {
	struct lock_descriptor_table *ptr;
	int i, j;
	for (i = 0; i < NLOCKS; i ++) {
		ptr = &lentry[i];
		ptr->lprio = -1;
		ptr->lstate = LFREE;
		ptr->iteration = 0;
		ptr->readers_count = 0;
		ptr->writers_count = 0;
		ptr->lreader_tail = 1 + (ptr->lreader_head = newqueue_semr());
		ptr->lwriter_tail = 1 + (ptr->lwriter_head = newqueue_semw());
		for (j = 0; j < NPROC; j ++) {
			ptr->process_bit_mask[j] = 0;
		}
	}
}

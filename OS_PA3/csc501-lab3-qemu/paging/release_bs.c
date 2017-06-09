#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {
	/* release the backing store with ID bs_id */
	STATWORD ps;
	disable(ps);
	
	if (bs_id < 0 || bs_id >= NUM_OF_BACKING_STORE) {
		restore(ps);
		return SYSERR;
	}
	
	// For releasing the backstore
	// iterate through all the processes 
	// and identify bs_id and bsm_unmap them
	int i;
	for (i = 0; i < NPROC; i ++) {
		bs_list *data_list = NULL;
		bs_list *p; 
		struct pentry *pptr;
		pptr = &proctab[i];
		
		if (pptr->list == NULL)
			continue;
		
		bs_list *it;
		it = proctab[i].list;
		
		// The data list to call bsm_unmap
		while (it != NULL) {
			if (it->bs_id == bs_id) {
				if (data_list == NULL) {
					data_list = getmem(sizeof(bs_list));
					data_list->bs_id = bs_id;
					data_list->bs_map = it->bs_map;
					data_list->next = NULL;
					p = data_list;
				} else {
					bs_list *node = getmem(sizeof(bs_list));
					node->bs_id = bs_id;
					node->bs_map = it->bs_map;
					node->next = NULL;
					p->next = node;
					p = p->next;
				}
			}
			it = it->next;
		}
		
		if (data_list == NULL) {
			continue;
		} else {
			bs_list *z;
			z = data_list;
			
			while (z != NULL) {
				//bsm_unmap(int pid, int vpno, int flag)
				bsm_unmap(i, z->bs_map.bs_vpno, 0);
				z = z->next;
			}
		}
		freemem(data_list, sizeof(bs_list));
	}
	
	free_bsm(bs_id);
	
	restore(ps);
	return OK;
}


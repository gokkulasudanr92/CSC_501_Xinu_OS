/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

bs_map_t bsm_tab[NUM_OF_BACKING_STORE];
fr_map_t frm_tab[NFRAMES];
/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{ 
	int i;
	for (i = 0; i < NUM_OF_BACKING_STORE; i ++) {
		bsm_tab[i].bs_status = BSM_UNMAPPED;			/* MAPPED or UNMAPPED		*/
		bsm_tab[i].bs_pid = -1;							/* process id using this slot   */
		bsm_tab[i].bs_vpno = -1;						/* starting virtual page number */
		bsm_tab[i].bs_npages = MAXPG;					/* number of pages in the store */
		bsm_tab[i].bs_sem = -1;
		bsm_tab[i].bs_private_status = NON_PRIVATE;			/* private access status */
		bsm_tab[i].bs_ref_count = 0;
		
		int j;
		for (j = 0; j < MAXPG; j ++) {
			bsm_tab[i].bs_page_frame_no[j] = -1;
		}
	}
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	STATWORD ps;
	disable(ps);
	
	int i;
	for (i = 0; i < NUM_OF_BACKING_STORE; i ++) {
		if (bsm_tab[i].bs_status == BSM_UNMAPPED) {
			restore(ps);
			return i;
		}
	}
	
	restore(ps);
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	STATWORD ps;
	disable(ps);
	
	if (i < 0 || i >= NUM_OF_BACKING_STORE) {
		restore(ps);
		return SYSERR;
	}
	
	if (bsm_tab[i].bs_ref_count > 0) {
		restore(ps);
		return SYSERR;
	}
	
	bsm_tab[i].bs_status = BSM_UNMAPPED;
	bsm_tab[i].bs_pid = -1;
	bsm_tab[i].bs_vpno = -1;
	bsm_tab[i].bs_npages = MAXPG;
	bsm_tab[i].bs_sem = -1;
	bsm_tab[i].bs_private_status = NON_PRIVATE;
	bsm_tab[i].bs_ref_count = 0;
	
	int j;
	for (j = 0; j < MAXPG; j ++) {
		bsm_tab[i].bs_page_frame_no[j] = -1;
	}
	//int j;
	//for(j = 0; j < NPROC; j ++) {
	//	proctab[j].backing_store_data[i].bs_status = BSM_UNMAPPED;			/* MAPPED or UNMAPPED		*/
	//	proctab[j].backing_store_data[i].bs_pid = -1;							/* process id using this slot   */
	//	proctab[j].backing_store_data[i].bs_vpno = -1;						/* starting virtual page number */
	//	proctab[j].backing_store_data[i].bs_npages = MAXPG;					/* number of pages in the store */
	//	proctab[j].backing_store_data[i].bs_sem = -1;
	//	proctab[j].backing_store_data[i].bs_private_status = NON_PRIVATE;			/* private access status */
	//	proctab[j].backing_store_data[i].bs_ref_count = 0;
	//}
	
	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, unsigned long vaddr, int* store, int* pageth)
{
	STATWORD ps;
	disable(ps);
	
	int i;
	unsigned long virtual_page_number = vaddr / 4096;
	
	struct pentry *pptr = &proctab[pid];
	// There is no mapping for the store
	if (pptr->list == NULL) {
		restore(ps);
		return SYSERR;
	}
	
	bs_list *it = pptr->list;
	while (it != NULL) {
		if (it->bs_map.bs_vpno <= virtual_page_number && 
		(it->bs_map.bs_vpno + it->bs_map.bs_npages) > virtual_page_number && 
		bsm_tab[it->bs_id].bs_status == BSM_MAPPED) {
			*store = it->bs_id;
			*pageth = virtual_page_number - it->bs_map.bs_vpno;
			restore(ps);
			return OK;
		}
		it = it->next;
	}
	
	//for (i = 0; i < NUM_OF_BACKING_STORE; i ++) {
	//	if (proctab[pid].backing_store_data[i].bs_pid == pid &&
	//	bsm_tab[i].bs_vpno <= virtual_page_number && (bsm_tab[i].bs_vpno + bsm_tab[i].bs_npages) > virtual_page_number &&
	//	bsm_tab[i].bs_status == BSM_MAPPED) {
	//		*store = i;
	//		*pageth = virtual_page_number - bsm_tab[i].bs_vpno;
	//		restore(ps);
	//		return OK;
	//	}
	//}
	
	restore(ps);
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	STATWORD ps;
	disable(ps);
	
	// check for the parameters
	if (vpno < 4096 || source < 0 || source > 7 || npages < 0 || npages > MAXPG || 
	(bsm_tab[source].bs_status == BSM_MAPPED && bsm_tab[source].bs_private_status == PRIVATE)) {
		kprintf("Invalid parameters for the bsm_map syscall.\n");
		restore(ps);
		return SYSERR;
	}
	
	// Assign the bsm map if it is unmapped
	if (bsm_tab[source].bs_status == BSM_UNMAPPED) {
		bsm_tab[source].bs_status = BSM_MAPPED;
		bsm_tab[source].bs_pid = pid;
		bsm_tab[source].bs_npages = npages;
		bsm_tab[source].bs_vpno = vpno;
	} else {
		if (bsm_tab[source].bs_pid == -1)
			bsm_tab[source].bs_pid = pid;
		if (bsm_tab[source].bs_vpno == -1)
			bsm_tab[source].bs_vpno = vpno;
	}
	bsm_tab[source].bs_private_status = NON_PRIVATE;
	
	// If pptr->list == NULL
	struct	pentry *pptr;
	pptr = &proctab[pid];
	
	if (pptr->list == NULL) {
		pptr->list = getmem(sizeof(bs_list));
		pptr->list->bs_id = source;
		pptr->list->bs_map.bs_status = BSM_MAPPED;							/* MAPPED or UNMAPPED*/
		pptr->list->bs_map.bs_pid = pid;										/* process id using this slot */
		pptr->list->bs_map.bs_vpno = vpno;									/* starting virtual page number */
		pptr->list->bs_map.bs_npages = bsm_tab[source].bs_npages;				/* number of pages in the store */
		pptr->list->bs_map.bs_sem = 0;
		pptr->list->bs_map.bs_private_status = NON_PRIVATE;					/* private access status */
		pptr->list->bs_map.bs_ref_count = bsm_tab[source].bs_ref_count;
		pptr->list->next = NULL;
	} else {
		// Check the backing store for the same pid and backing store id
		// and virtpage number
		int is_not_pres = 1, flag = 1;
		bs_list *it;
		it = pptr->list;
		
		while (it->next != NULL) {
			if (it->bs_id == source && 
			it->bs_map.bs_pid == pid && 
			it->bs_map.bs_vpno == vpno) {
				is_not_pres = 0;
				flag = 0;
			}
			it = it->next;
		}
		
		// Check the last element also
		if (flag && it->bs_id == source && 
		it->bs_map.bs_pid == pid && 
		it->bs_map.bs_vpno == vpno) {
			is_not_pres = 0;
		}
		
		// If is_not_pres then add the data to the list, else neglect the reference the data is already there
		if (is_not_pres) {
			bsm_tab[source].bs_ref_count ++;
			bs_list *node = getmem(sizeof(bs_list));
			node->bs_id = source;
			node->bs_map.bs_status = BSM_MAPPED;							/* MAPPED or UNMAPPED*/
			node->bs_map.bs_pid = pid;										/* process id using this slot */
			node->bs_map.bs_vpno = vpno;									/* starting virtual page number */
			node->bs_map.bs_npages = bsm_tab[source].bs_npages;				/* number of pages in the store */
			node->bs_map.bs_sem = 0;
			node->bs_map.bs_private_status = NON_PRIVATE;					/* private access status */
			node->bs_map.bs_ref_count = bsm_tab[source].bs_ref_count;
			node->next = NULL;
			it->next = node;
			//proctab[pid].backing_store_data[source].bs_status = BSM_MAPPED;			/* MAPPED or UNMAPPED		*/
			//proctab[pid].backing_store_data[source].bs_pid = pid;							/* process id using this slot   */
			//proctab[pid].backing_store_data[source].bs_vpno = vpno;						/* starting virtual page number */
			//proctab[pid].backing_store_data[source].bs_npages = bsm_tab[source].bs_npages;					/* number of pages in the store */
			//proctab[pid].backing_store_data[source].bs_sem = 0;
			//proctab[pid].backing_store_data[source].bs_private_status = NON_PRIVATE;			/* private access status */
			//proctab[pid].backing_store_data[source].bs_ref_count = bsm_tab[source].bs_ref_count;
		}
	}
	
	restore(ps);
	return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	STATWORD ps;
	disable(ps);
	
	int store, pageth;
	if (bsm_lookup(pid, vpno * NBPG, &store, &pageth) == SYSERR) {
		kprintf("SYSERR - Backstore not mapped to virtual address\n");
		restore(ps);
		return SYSERR;
	}
	
	if (flag) {
		proctab[pid].vmemlist = NULL;
	} else {
		// Need to release the frame
		unsigned long pd_offset = vpno / 1024;
		unsigned long pt_offset = vpno % 1024;
		
		pd_t *directory_entry = proctab[pid].pdbr + (pd_offset * sizeof(pd_t));
		
		int i;
		for (i = 0; i < bsm_tab[store].bs_npages; i ++) {
			pt_t *table_entry = directory_entry->pd_base * NBPG + ((pt_offset + i) * sizeof(pt_t));
			
			int frame = table_entry->pt_base - FRAME0;
			if (frame < 1024) {
				if (frm_tab[frame].fr_status == FRM_MAPPED && is_single_reference(frame)) {
					free_frm(frame);
				} else {
					// Clear page table entry
					table_entry->pt_acc = 0;
					table_entry->pt_avail = 0;
					table_entry->pt_base = 0;
					table_entry->pt_dirty = 0;
					table_entry->pt_global = 0;
					table_entry->pt_mbz = 0;
					table_entry->pt_pcd = 0;
					table_entry->pt_pres = 0;
					table_entry->pt_pwt = 0;
					table_entry->pt_write = 1;
					table_entry->pt_user = 0;
					
					//**** clear page table entry if ref_count = 0 from frm_tab also clear directory specific entry pd_t also
					frm_tab[((unsigned long)table_entry >> 12) - FRAME0].fr_refcnt --;
					if (frm_tab[((unsigned long)table_entry >> 12) - FRAME0].fr_refcnt == 0) {
						frm_tab[((unsigned long)table_entry >> 12) - FRAME0].fr_dirty = 0;
						frm_tab[((unsigned long)table_entry >> 12) - FRAME0].fr_pid = -1;
						frm_tab[((unsigned long)table_entry >> 12) - FRAME0].fr_refcnt = 0;
						frm_tab[((unsigned long)table_entry >> 12) - FRAME0].fr_status = FRM_UNMAPPED;
						frm_tab[((unsigned long)table_entry >> 12) - FRAME0].fr_type = FR_PAGE;
						frm_tab[((unsigned long)table_entry >> 12) - FRAME0].fr_vpno = -1;
				
						directory_entry->pd_pres = 0;
						directory_entry->pd_write = 1;
						directory_entry->pd_user = 0;
						directory_entry->pd_pwt = 0;
						directory_entry->pd_pcd = 0;
						directory_entry->pd_acc = 0;
						directory_entry->pd_mbz = 0;
						directory_entry->pd_fmb = 0;
						directory_entry->pd_global = 0;
						directory_entry->pd_avail = 0;
						directory_entry->pd_base = 0;
					}
				}
			}
			
			// Remove frame entry from the frm_tab
			fr_map_t *fr = &frm_tab[frame];
			fr_pid_vpno_data *list = fr->list;
			
			while (list->next != NULL) {
				if (list->next->vpno == vpno && list->next->pid == pid) {
					fr_pid_vpno_data *delete_node = list->next;
					list->next = list->next->next;

					freemem(delete_node, sizeof(fr_pid_vpno_data));
					break;
				}
				list = list->next;
			}
		}
		directory_entry->pd_pres = 0;
		directory_entry->pd_write = 1;
		directory_entry->pd_user = 0;
		directory_entry->pd_pwt = 0;
		directory_entry->pd_pcd = 0;
		directory_entry->pd_acc = 0;
		directory_entry->pd_mbz = 0;
		directory_entry->pd_fmb = 0;
		directory_entry->pd_global = 0;
		directory_entry->pd_avail = 0;
		directory_entry->pd_base = 0;
	}
	
	bsm_tab[store].bs_ref_count --;
	
	//Remove the same entry from proctab bs_list entry
	bs_list *p, *q;
	struct pentry *pptr;
	pptr = &proctab[pid];
	p = pptr->list;
	
	if (p->bs_id == store && 
	p->bs_map.bs_pid == pid && 
	p->bs_map.bs_vpno == vpno) {
		pptr->list = p->next;
		freemem(p, sizeof(bs_list));
	} else {
		q = p->next;
		int flag = 1;
		while (q->next != NULL) {
			if (q->bs_id == store && 
			q->bs_map.bs_pid == pid && 
			q->bs_map.bs_vpno == vpno) {
				p->next = q->next;
				flag = 0;
				freemem(q, sizeof(bs_list));
				break;
			}
			p = p->next;
			q = q->next;
		}
		
		// Check last element for the match
		if (flag && q->bs_id == store && 
		q->bs_map.bs_pid == pid && 
		q->bs_map.bs_vpno == vpno) {
			p->next = q->next;
			freemem(q, sizeof(bs_list));
		}
	}
	
	//proctab[pid].backing_store_data[store].bs_status = BSM_UNMAPPED;			/* MAPPED or UNMAPPED		*/
	//proctab[pid].backing_store_data[store].bs_pid = -1;							/* process id using this slot   */
	//proctab[pid].backing_store_data[store].bs_vpno = -1;						/* starting virtual page number */
	//proctab[pid].backing_store_data[store].bs_npages = MAXPG;					/* number of pages in the store */
	//proctab[pid].backing_store_data[store].bs_sem = -1;
	//proctab[pid].backing_store_data[store].bs_private_status = NON_PRIVATE;			/* private access status */
	//proctab[pid].backing_store_data[store].bs_ref_count = 0;
	
	restore(ps);
	return OK;
}



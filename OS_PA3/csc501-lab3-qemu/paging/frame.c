/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

// 4 Global Page Tables are public to all the processes across 16 MB for a 4K page size
int global[4];
fr_map_t frm_tab[NFRAMES];
/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_frm()
{
	int i;
	for (i = 0; i < NFRAMES; i++) {
		fr_map_t *frame;
		frame = &frm_tab[i];
		frame->fr_status = FRM_UNMAPPED;
		frame->fr_pid = -1;
		frame->fr_vpno = -1;
		frame->fr_refcnt = 0;
		frame->fr_type = FR_PAGE;
		frame->fr_dirty = 0;
		
		// Dummy header for the mappings of the process pid
		// & corresponding vpno
		frame->list = getmem(sizeof(fr_pid_vpno_data));
		frame->list->pid = -1;
		frame->list->vpno = -1;
		frame->list->next = NULL;
	}
	return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
int get_frm()
{
	// If frame is free, then assign and return that frame number
	STATWORD ps;
	disable(ps);
	
	int i;
	for (i = 0; i < NFRAMES; i++) {
		if (frm_tab[i].fr_status == FRM_UNMAPPED) {
			frm_tab[i].fr_status = FRM_MAPPED;
			return i;
		}
	}
	
	int next_frame = SYSERR;
	if (grpolicy() == SC) {
		if (sc_current == NULL) {
			restore(ps);
			return SYSERR;
		}
		
		// First pass to identify the pt->acc
		// if all the 1's then only start the second pass
		while (1) {
			int fr_num = sc_current->frame_no;
			fr_pid_vpno_data *list = frm_tab[fr_num - FRAME0].list->next;
			int acc = 0;
			while (list != NULL) {
				unsigned long pd_offset = list->vpno / 1024;
				unsigned long pt_offset = list->vpno % 1024;
				
				pd_t *pd = proctab[list->pid].pdbr + (pd_offset * sizeof(pd_t));
				pt_t *pt = pd->pd_base * NBPG + (pt_offset * sizeof(pt_t));
				
				if (pt->pt_acc) {
					pt->pt_acc = 0;
					acc = 1;
				}
				list = list->next;
			}
			
			if (!acc) {
				next_frame = sc_current->frame_no - FRAME0;
				break;
			}
			sc_current = sc_current->next;
		}
		
		if (next_frame != SYSERR) {
			if (get_debug()) {
				kprintf("Replacing Frame No: %d\n", sc_current->frame_no);
			}
			free_frm(sc_current->frame_no - FRAME0, currpid);
			return next_frame;
		}
	} else if (grpolicy() == AGING) {
		// First pass to identify the pt->acc
		// if all the 1's update the age and also identify the min_age
		// to delete from the aging_queue list
		aging_queue *it = aging_head;
		while (it != NULL) {
			int fr_num = it->frame_no;
			fr_pid_vpno_data *list = frm_tab[fr_num - FRAME0].list->next;
			int acc = 0;
			while (list != NULL) {
				unsigned long pd_offset = list->vpno / 1024;
				unsigned long pt_offset = list->vpno % 1024;
				
				pd_t *pd = proctab[list->pid].pdbr + (pd_offset * sizeof(pd_t));
				pt_t *pt = pd->pd_base * NBPG + (pt_offset * sizeof(pt_t));
				
				if (pt->pt_acc) {
					pt->pt_acc = 0;
					acc = 1;
				}
				list = list->next;
			}
			
			it->age = it->age >> 1;
			if (acc) {
				it->age += 128;
			}
			it = it->next;
		}
		
		// Identify the minimum age frame to swap out
		int min_age = 256;
		it = aging_head;
		while (it != NULL) {
			if (min_age > it->age) {
				min_age = it->age;
				next_frame = it->frame_no;
			}
			it = it->next;
		}
		
		if (next_frame != SYSERR) {
			if (get_debug()) {
				kprintf("Replacing Frame No: %d\n", next_frame);
			}
			free_frm(next_frame - FRAME0, currpid);
			return next_frame - FRAME0;
		}
	}
	
	restore(ps);
	return SYSERR;
}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i, int pid)
{
  STATWORD ps;
  disable(ps);
  
  //int pid = frm_tab[i].fr_pid;
  if(i < 0 || i >= NFRAMES){
	restore(ps);
	return SYSERR;
  }
  
  switch(frm_tab[i].fr_type) {
	case FR_DIR: {
		int j;
		for (j = 4; j < 1024; j ++) {
			pd_t *directory_entry = proctab[pid].pdbr + (i * sizeof(pd_t));
			
			if (directory_entry->pd_pres) {
				free_frm(directory_entry->pd_base - FRAME0, pid);
			}
		}
		frm_tab[i].fr_status = FRM_UNMAPPED;
		frm_tab[i].fr_pid = -1;
		frm_tab[i].fr_vpno = -1;
		frm_tab[i].fr_refcnt = 0;
		frm_tab[i].fr_type = FR_PAGE;
		frm_tab[i].fr_dirty = 0;
			
		restore(ps);
		return OK;
	}
	case FR_TBL: {
		int j;
		for (j = 0; j < 1024; j ++) {
			pt_t *table_entry = (FRAME0 + i) * NBPG + (j * sizeof(pt_t));
			
			if (table_entry->pt_pres) {
				free_frm(table_entry->pt_base - FRAME0, pid);
			}
		}
		
		for (j = 0; j < 1024; j++) {
			pd_t *pd_entry = proctab[pid].pdbr + (j * sizeof(pd_t));
			if (pd_entry->pd_base - FRAME0 == i) {
				pd_entry->pd_pres = 0;
				pd_entry->pd_write = 1;
				pd_entry->pd_user = 0;
				pd_entry->pd_pwt = 0;
				pd_entry->pd_pcd = 0;
				pd_entry->pd_acc = 0;
				pd_entry->pd_mbz = 0;
				pd_entry->pd_fmb = 0;
				pd_entry->pd_global = 0;
				pd_entry->pd_avail = 0;
				pd_entry->pd_base = 0;
			}
		}
		
		frm_tab[i].fr_status = FRM_UNMAPPED;
		frm_tab[i].fr_pid = -1;
		frm_tab[i].fr_vpno = -1;
		frm_tab[i].fr_refcnt = 0;
		frm_tab[i].fr_type = FR_PAGE;
		frm_tab[i].fr_dirty = 0;
		
		restore(ps);
		return OK;
	}
	case FR_PAGE: {
		int store;
		int pageth;
		long vpno;
		
		fr_map_t *fr = &frm_tab[i];
		fr_pid_vpno_data *node = fr->list->next;
		
		while (node != NULL) {
			vpno = node->vpno;
			pid = node->pid;
			
			pd_t *pd_entry = proctab[pid].pdbr + (vpno >> 10) * sizeof(pd_t);
			pt_t *pt_entry = (pd_entry->pd_base * NBPG) + ((vpno % 1024) * sizeof(pt_t));
			
			if (!pt_entry->pt_pres || bsm_lookup(pid, vpno * 4096, &store, &pageth) == SYSERR) {
				restore(ps);
				return SYSERR;
			}
			
			unsigned long frame_addr = (FRAME0 + i) * NBPG;
			write_bs(frame_addr, store, pageth);
			
			pt_entry->pt_acc = 0;
			pt_entry->pt_avail = 0;
			pt_entry->pt_base = 0;
			pt_entry->pt_dirty = 0;
			pt_entry->pt_global = 0;
			pt_entry->pt_mbz = 0;
			pt_entry->pt_pcd = 0;
			pt_entry->pt_pres = 0;
			pt_entry->pt_pwt = 0;
			pt_entry->pt_write = 1;
			pt_entry->pt_user = 0;
			
			//***** clear page table entry if ref_count = 0 from frm_tab also clear directory specific entry pd_t also
			frm_tab[((unsigned long)pt_entry >> 12) - FRAME0].fr_refcnt --;
			if (frm_tab[((unsigned long)pt_entry >> 12) - FRAME0].fr_refcnt == 0) {
				frm_tab[((unsigned long)pt_entry >> 12) - FRAME0].fr_dirty = 0;
				frm_tab[((unsigned long)pt_entry >> 12) - FRAME0].fr_pid = -1;
				frm_tab[((unsigned long)pt_entry >> 12) - FRAME0].fr_refcnt = 0;
				frm_tab[((unsigned long)pt_entry >> 12) - FRAME0].fr_status = FRM_UNMAPPED;
				frm_tab[((unsigned long)pt_entry >> 12) - FRAME0].fr_type = FR_PAGE;
				frm_tab[((unsigned long)pt_entry >> 12) - FRAME0].fr_vpno = -1;
				
				pd_entry->pd_pres = 0;
				pd_entry->pd_write = 1;
				pd_entry->pd_user = 0;
				pd_entry->pd_pwt = 0;
				pd_entry->pd_pcd = 0;
				pd_entry->pd_acc = 0;
				pd_entry->pd_mbz = 0;
				pd_entry->pd_fmb = 0;
				pd_entry->pd_global = 0;
				pd_entry->pd_avail = 0;
				pd_entry->pd_base = 0;
			}
			node = node->next;
		}
		
		// Also remove the frame from the replacement queue
		if (grpolicy() == SC) {
			sc_circular_queue *node = sc_head;
			
			while (node->next != sc_current) {
				node = node->next;
			}
			
			sc_circular_queue *delete = sc_current;
			
			node->next = delete->next;
			if (delete == sc_head) {
				if (sc_head->next != sc_head) {
					sc_head = sc_head->next;
				} else {
					sc_head = NULL;
					sc_current = NULL;
				}
			}
			sc_current = delete->next;
			freemem(delete, sizeof(sc_circular_queue));
		} else if (grpolicy() == AGING) {
			// Find the node to delete with frame number
			aging_queue *node = aging_head;
			aging_queue *it = aging_head->next;
			
			if (node->frame_no == (FRAME0 + i)) {
				aging_head = node->next;
				freemem(node, sizeof(aging_queue));
			} else {
				while (it != NULL) {
					if (it->frame_no == (FRAME0 + i)) {
						break;
					}
					it = it->next;
					node = node->next;
				}
				
				node->next = it->next;
				freemem(it, sizeof(aging_queue));
			}
		} else {
			restore(ps);
			return SYSERR;
		}
		
		frm_tab[i].fr_status = FRM_UNMAPPED;
		frm_tab[i].fr_pid = -1;
		frm_tab[i].fr_vpno = -1;
		frm_tab[i].fr_refcnt = 0;
		frm_tab[i].fr_type = FR_PAGE;
		frm_tab[i].fr_dirty = 0;
		fr_map_t *x = &frm_tab[i];
		fr_pid_vpno_data *ptr = x->list;
		
		while (ptr->next != NULL) {
			fr_pid_vpno_data *delete = ptr->next;
			ptr->next = delete->next;
			freemem(delete, sizeof(fr_pid_vpno_data));
		}
		bsm_tab[store].bs_page_frame_no[pageth] = -1;

		restore(ps);
  		return OK;
	}
  }
  restore(ps);
  return SYSERR;
}

/*-----------------------------------------------------------------------------------
 * create page table - Page Table is created for the given pid in global page table
 *-----------------------------------------------------------------------------------
 */
int create_page_table(int pid) {
	int i;
	int frame = get_frm();
	if (frame == SYSERR) {
		return SYSERR;
	}
	
	frm_tab[frame].fr_status = FRM_MAPPED;
	frm_tab[frame].fr_pid = pid;
	frm_tab[frame].fr_vpno = -1;
	frm_tab[frame].fr_refcnt = 0;
	frm_tab[frame].fr_type = FR_TBL;
	frm_tab[frame].fr_dirty = 0;
	
	for (i = 0; i < 1024; i++) {
		pt_t *pt = (FRAME0 + frame) * NBPG + (i * sizeof(pt_t));
		pt->pt_pres = 0;
		pt->pt_write = 1;
		pt->pt_user = 0;
		pt->pt_pwt = 0;
		pt->pt_pcd = 0;
		pt->pt_acc = 0;
		pt->pt_dirty = 0;
		pt->pt_mbz = 0;
		pt->pt_global = 0;
		pt->pt_avail = 0;
		pt->pt_base = 0;		
	}
	return frame;
}

int create_page_directory(int pid) {
	int i;
	int frame = get_frm();
	if (frame == SYSERR) {
		return SYSERR;
	}
	
	frm_tab[frame].fr_status = FRM_MAPPED;
	frm_tab[frame].fr_pid = pid;
	frm_tab[frame].fr_vpno = -1;
	frm_tab[frame].fr_refcnt = 4;
	frm_tab[frame].fr_type = FR_DIR;
	frm_tab[frame].fr_dirty = 0;
	
	// Update Directory Entry in Proctab struct
	proctab[pid].pdbr = (FRAME0 + frame) * NBPG;
	unsigned long pdbr = proctab[pid].pdbr;
	for (i = 0; i < 1024; i ++) {
		pd_t *pd_data = pdbr + (i * sizeof(pd_t));
		
		// For the first 4 global tables the directory is writable and
		// present with page entry
		if (i < 4) {
			pd_data->pd_pres = 1; 		/* Page entry is present */
			pd_data->pd_base = global[i];
		} else {
			pd_data->pd_pres = 0; 		/* Page entry is present */
			pd_data->pd_base = 0;
		}
		
		pd_data->pd_user = 0;
		pd_data->pd_pwt = 0;
		pd_data->pd_pcd	= 0;
		pd_data->pd_acc = 0;
		pd_data->pd_mbz	= 0;
		pd_data->pd_fmb	= 0;
		pd_data->pd_write = 1;		/* Page is present */
		pd_data->pd_global = 0;
		pd_data->pd_avail = 0;
	}
	
	return frame;
}

int init_page_directory_NULLPROC() {
	return create_page_directory(NULLPROC);
}

/*-------------------------------------------------------------------------------
 *  initialize the global page table which is common to all processes
 *-------------------------------------------------------------------------------
 */
int initialize_global() {
	int i, j, base_frame;
	for (i = 0; i < 4; i++) {
		base_frame = create_page_table(NULLPROC);
		if (base_frame == SYSERR) {
			return SYSERR;
		}
		global[i] = FRAME0 + base_frame;

		for (j = 0; j < 1024; j++) {
			pt_t *pt = global[i] * NBPG + (j * sizeof(pt_t));

			pt->pt_pres = 1;
			pt->pt_write = 1;
			pt->pt_base = j + i * 1024;

			frm_tab[base_frame].fr_refcnt++;
		}
	}
	return OK;
}

int is_single_reference(int frame_number) {
	fr_map_t *fr = &frm_tab[frame_number];
	fr_pid_vpno_data *list = fr->list;
	if (list->next->next == NULL)
		return 1;
	return 0;
}

int rem_frames() {
	int sum = 0, i;
	for (i = 0; i < NFRAMES; i ++) {
		if (frm_tab[i].fr_status == FRM_UNMAPPED)
			sum += 1;
	}
	return sum;
}


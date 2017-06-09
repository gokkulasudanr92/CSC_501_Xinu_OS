/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */
SYSCALL pfint()
{
  STATWORD ps;
  disable(ps);
  
  // Obtain faulted address
  unsigned long fault_address = read_cr2();
  unsigned long vpno = fault_address / 4096;
  unsigned long pd_offset = fault_address >> 22;
  
  int store =-1, pageth=-1;
  pd_t *directory_table = proctab[currpid].pdbr + (pd_offset * sizeof(pd_t));
  if (bsm_lookup(currpid, fault_address, &store, &pageth) == SYSERR || store == -1 || pageth == -1) {
	kprintf("pfint.c: SYSERR - Backstore not mapped to virtual address.\n");
	kill(currpid);
	restore(ps);
	return SYSERR;
  }
  
  // If directory is not present we assign a new frame for
  // page table
  if (directory_table->pd_pres == 0) {
		int new_frame = create_page_table(currpid);
		if (new_frame == -1) {
			restore(ps);
			return SYSERR;
		}
		
		directory_table->pd_pres = 1;
		directory_table->pd_write = 1;
		directory_table->pd_base = new_frame + FRAME0;
		unsigned long entry_index = (unsigned) directory_table;
		entry_index = entry_index / NBPG;
		entry_index -= FRAME0;
		
		frm_tab[entry_index].fr_refcnt++;
	}
	
	unsigned long pt_offset = (unsigned long) fault_address;
	pt_offset = pt_offset / NBPG;
	pt_offset = pt_offset & 0x000003ff;
	pt_t *table_entry = directory_table->pd_base * NBPG + pt_offset * sizeof(pt_t);
	
	int next_frame;
	if (bsm_tab[store].bs_page_frame_no[pageth] == -1) {
		next_frame = get_frm();
		read_bs((FRAME0 + next_frame) * NBPG, store, pageth);
		bsm_tab[store].bs_page_frame_no[pageth] = next_frame;
		frm_tab[next_frame].fr_refcnt = 1;
		frm_tab[next_frame].fr_status = FRM_MAPPED;
	} else {
		next_frame = bsm_tab[store].bs_page_frame_no[pageth];
		frm_tab[next_frame].fr_refcnt ++;
	}
	
	if (next_frame == SYSERR) {
		kprintf("SYSERR - No frame available\n");
		kill(currpid);
		restore(ps);
		return SYSERR;
	}
	
	frm_tab[next_frame].fr_status = FRM_MAPPED;
	frm_tab[next_frame].fr_pid = currpid;
	frm_tab[next_frame].fr_vpno = vpno;
	frm_tab[next_frame].fr_type = FR_PAGE;
	
	//fr_map_t *fr_ptr;
	//fr_ptr = &frm_tab[next_frame - FRAME0];
	
	//fr_pid_vpno_data *list = fr_ptr->list;
	//int flag = 1;
	//while (list->next != NULL) {
	//	if (list->next->pid == currpid && 
	//	list->next->vpno == vpno) {
	//		flag = 0;
	//		break;
	//	}
	//	list = list->next;
	//}
	  
	//if (flag) {
	//	fr_pid_vpno_data *node = getmem(sizeof(fr_pid_vpno_data));
	//	node->pid = currpid;
	//	node->vpno = vpno;
	//	node->next = NULL;
	//	list->next = node;
	//}
	
	// Add the frame number to the corresponding queue
	if (grpolicy() == SC) {
		if (sc_head == NULL) {
			sc_head = getmem(sizeof(sc_circular_queue));
			sc_head->frame_no = FRAME0 + next_frame;
			sc_head->next = sc_head;
			sc_current = sc_head;
		} else {
			sc_circular_queue *node = sc_head;
			while (node->next != sc_head) {
				node = node->next;
			}
			sc_circular_queue *new_node = getmem(sizeof(sc_circular_queue));
			new_node->frame_no = next_frame + FRAME0;
			new_node->next = sc_head;
			node->next = new_node;
		}
	} else if (grpolicy() == AGING) {
		if (aging_head == NULL) {
			aging_head = getmem(sizeof(aging_queue));
			aging_head->frame_no = FRAME0 + next_frame;
			aging_head->age = 0;
			aging_head->next = NULL;
		} else {
			// Add the list to the end of the list
			// since it is a FIFO Queue
			aging_queue *it = aging_head;
			while (it->next != NULL) {
				it = it->next;
			}
			aging_queue *node = getmem(sizeof(aging_queue));
			node->frame_no = FRAME0 + next_frame;
			node->age = 0;
			node->next = NULL;
			it->next = node;
		}
	} else {
		restore(ps);
		return SYSERR;
	}
	
	table_entry->pt_base = FRAME0 + next_frame;
	table_entry->pt_pres = 1;
	table_entry->pt_write = 1;
	//unsigned long index = (unsigned long) table_entry;
	//index = index / 4096;
	int index = next_frame;
	frm_tab[index].fr_refcnt ++;
	/////////////////
	fr_map_t *fr_ptr;
	  fr_ptr = &frm_tab[index];
	  
	  fr_pid_vpno_data *list = fr_ptr->list;
	  int flag = 1;
	  while (list->next != NULL) {
		  if (list->next->pid == currpid && 
		  list->next->vpno == vpno) {
			  flag = 0;
			  break;
		  }
		  list = list->next;
	  }
	  
	  if (flag) {
		  fr_pid_vpno_data *node = getmem(sizeof(fr_pid_vpno_data));
		  node->pid = currpid;
		  node->vpno = vpno;
		  node->next = NULL;
		  list->next = node;
	  }
	  //////////////////////
	
	write_cr3(proctab[currpid].pdbr);
	
	restore(ps);
	return OK;
}



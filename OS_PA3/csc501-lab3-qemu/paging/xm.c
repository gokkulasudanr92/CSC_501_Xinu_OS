/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

bs_map_t bsm_tab[NUM_OF_BACKING_STORE];
fr_map_t frm_tab[NFRAMES];
/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  STATWORD ps;
  disable(ps);
  
  // check for the parameters
  if (virtpage < 4096 || source < 0 || source > 7 || npages < 1 || npages > MAXPG) {
	  kprintf("Invalid Parameters for the xmap syscall.\n");
	  restore(ps);
	  return SYSERR;
  }
  
  bs_map_t *ptr = &bsm_tab[source];
  if ((ptr->bs_status == BSM_UNMAPPED) || (ptr->bs_private_status == PRIVATE)) {
	  kprintf("The backstore is unmapped or it is a private heap\n");
	  restore(ps);
	  return SYSERR;
  }
  
  if (npages > ptr->bs_npages) {
	  kprintf("The pages trying to map is beyond the backstore allocated limit\n");
	  restore(ps);
	  return SYSERR;
  }
  
  if (bsm_map(currpid, virtpage, source, npages) == SYSERR) {
	  kprintf("Error while performing the bsm map for the user defined backstore\n");
	  restore(ps);
	  return SYSERR;
  }
  
  unsigned long directory_offset = virtpage / 1024;
  unsigned long table_offset = virtpage % 1024;
  
  pd_t *directory_entry = proctab[currpid].pdbr + (directory_offset * sizeof(pd_t));
  // If page directory is not present create a new one with the available frames
  if (directory_entry->pd_pres == 0) {
	  int table_frame = create_page_table(currpid);
	  if (table_frame == SYSERR) {
		kprintf("Unable to allocate frame for the current process. Iniating kill for current process ...");
		kill(currpid);
		restore(ps);
		return SYSERR;  
	  }
	  
	  // Creation of page table in a frame was successful
	  directory_entry->pd_base = FRAME0 + table_frame;
	  directory_entry->pd_pres = 1;
	  directory_entry->pd_write = 1;
	  unsigned long entry_index = (unsigned long) directory_entry / 4096;
	  entry_index -= FRAME0;
	  frm_tab[entry_index].fr_refcnt ++;
  }
  
  // We have page table available for the directory but we need to map it to the backing store
  // in xmmap
  pt_t *table_entry;
  int i;
  for (i = 0; i < npages; i ++) {
	  table_entry = directory_entry->pd_base * NBPG + ((i + table_offset) * sizeof(pt_t));
	  if (bsm_tab[source].bs_page_frame_no[i] == -1) {
		//table_entry->pt_base = BACKING_STORE_START + (source * MAXPG) + i;
		int fr = get_frm();
		
		if (fr == SYSERR) {
			kprintf("Unable to obtain the frame number \n");
			restore(ps);
			return SYSERR;
		}
		
		table_entry->pt_base = FRAME0 + fr;
		bsm_tab[source].bs_page_frame_no[i] = FRAME0 + fr;
		read_bs(bsm_tab[source].bs_page_frame_no[i] * NBPG, source, i);
		
		// Add the page to the replacement policy queue
		if (grpolicy() == SC) {
			if (sc_head == NULL) {
				sc_head = getmem(sizeof(sc_circular_queue));
				sc_head->frame_no = FRAME0 + fr;
				sc_head->next = sc_head;
				sc_current = sc_head;
			} else {
				// Add the list to the end of the list
				// since it is a FIFO Queue
				sc_circular_queue *it = sc_head;
				while (it->next != sc_head) {
					it = it->next;
				}
				sc_circular_queue *node = getmem(sizeof(sc_circular_queue));
				node->frame_no = FRAME0 + fr;
				node->next = sc_head;
				it->next = node;
			}
		} else if (grpolicy() == AGING) {
			if (aging_head == NULL) {
				aging_head = getmem(sizeof(aging_queue));
				aging_head->frame_no = FRAME0 + fr;
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
				node->frame_no = FRAME0 + fr;
				node->age = 0;
				node->next = NULL;
				it->next = node;
			}
		} else {
			restore(ps);
			return SYSERR;
		}
	  } else {
		table_entry->pt_base = bsm_tab[source].bs_page_frame_no[i];
	  }
	  
	  table_entry->pt_pres = 1;
	  table_entry->pt_write = 1;
	  
	  frm_tab[bsm_tab[source].bs_page_frame_no[i] - FRAME0].fr_status = FRM_MAPPED;
	  frm_tab[bsm_tab[source].bs_page_frame_no[i] - FRAME0].fr_refcnt ++;
	  fr_map_t *fr_ptr;
	  fr_ptr = &frm_tab[bsm_tab[source].bs_page_frame_no[i] - FRAME0];
	  
	  fr_pid_vpno_data *list = fr_ptr->list;
	  int flag = 1;
	  while (list->next != NULL) {
		  if (list->next->pid == currpid && 
		  list->next->vpno == (virtpage + i)) {
			  flag = 0;
			  break;
		  }
		  list = list->next;
	  }
	  
	  if (flag) {
		  fr_pid_vpno_data *node = getmem(sizeof(fr_pid_vpno_data));
		  node->pid = currpid;
		  node->vpno = (virtpage + i);
		  node->next = NULL;
		  list->next = node;
	  }
  }
  
  write_cr3(proctab[currpid].pdbr);
  
  restore(ps);
  return OK;
}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
	STATWORD ps;
	disable(ps);
	
	if (virtpage < 4096) {
		restore(ps);
		return SYSERR;
	}
	
	if (bsm_unmap(currpid, virtpage, 0) == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	
	write_cr3(proctab[currpid].pdbr);
	restore(ps);
	return OK;
}

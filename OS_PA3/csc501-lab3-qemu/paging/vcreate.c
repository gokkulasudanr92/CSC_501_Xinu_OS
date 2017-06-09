/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/
bs_map_t bsm_tab[NUM_OF_BACKING_STORE];
LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	disable(ps);
	
	if (hsize > MAXPG) {
		restore(ps);
		return SYSERR;
	}
	
	int bs_id = get_bsm();
	if(bs_id == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	
	int pid = create(procaddr, ssize, priority, name, nargs, args);
	if(pid == SYSERR){
		restore(ps);
		return SYSERR;
	}
	
	if (bsm_map(pid, 4096, bs_id, hsize) == SYSERR) {
		restore(ps);
		return SYSERR;
	}
	bsm_tab[bs_id].bs_private_status = PRIVATE;
	// Adding the mapping for 4096 virtual address;
	unsigned long virtpage = 4096;
	unsigned long directory_offset = virtpage / 1024;
	
	pd_t* directory_entry = proctab[pid].pdbr + (directory_offset * sizeof(pd_t));
	if(directory_entry->pd_pres == 0) {
		int table_frame = create_page_table(pid);;
		if(table_frame == SYSERR){
			kprintf("Unable to allocate frame for the current process. Iniating kill for current process ...");
			kill(pid);
			restore(ps);
			return SYSERR;
		}
		
	  	directory_entry->pd_base = FRAME0 + table_frame;
	  	directory_entry->pd_pres = 1;
	  	directory_entry->pd_write = 1;
		unsigned long entry_index = (unsigned long) directory_entry / 4096;
		entry_index -= FRAME0;
	  	frm_tab[entry_index].fr_refcnt++;
	}
	
	int i;
	for (i = 0; i < hsize; i ++) {
		pt_t *table_entry = directory_entry->pd_base * NBPG + (i * sizeof(pt_t));
		
		////////////////
		if (bsm_tab[bs_id].bs_page_frame_no[i] == -1) {
			int fr = get_frm();
		
			if (fr == SYSERR) {
				kprintf("Unable to obtain the frame number \n");
				restore(ps);
				return SYSERR;
			}
		
			table_entry->pt_base = FRAME0 + fr;
			bsm_tab[bs_id].bs_page_frame_no[i] = FRAME0 + fr;
			read_bs(bsm_tab[bs_id].bs_page_frame_no[i] * NBPG, bs_id, i);
		
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
			frm_tab[bsm_tab[bs_id].bs_page_frame_no[i] - FRAME0].fr_pid = pid;
			frm_tab[bsm_tab[bs_id].bs_page_frame_no[i] - FRAME0].fr_status = FRM_MAPPED;
			frm_tab[bsm_tab[bs_id].bs_page_frame_no[i] - FRAME0].fr_type = FR_PAGE;
		} else {
			table_entry->pt_base = bsm_tab[bs_id].bs_page_frame_no[i];
		}
		
		table_entry->pt_pres = 1;
		table_entry->pt_write = 1;
		
		frm_tab[bsm_tab[bs_id].bs_page_frame_no[i] - FRAME0].fr_refcnt ++;
		fr_map_t *fr_ptr;
		fr_ptr = &frm_tab[bsm_tab[bs_id].bs_page_frame_no[i] - FRAME0];
	  
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
		////////////////
		
		//table_entry->pt_base = BACKING_STORE_START + (bs_id * MAXPG) + i;
		//table_entry->pt_pres = 1;
		//table_entry->pt_write = 1;
		//unsigned long index = (unsigned long) table_entry / 4096;
		//index -= FRAME0;
		//frm_tab[index].fr_refcnt ++;
		//frm_tab[index].fr_vpno = (virtpage + i);
		
		//fr_map_t *fr_ptr;
		//fr_ptr = &frm_tab[index];
	  
		//fr_pid_vpno_data *list = fr_ptr->list;
		//int flag = 1;
		//while (list->next != NULL) {
		//	if (list->next->pid == currpid && 
		//	list->next->vpno == (virtpage + i)) {
		//		flag = 0;
		//		break;
		//	}
		//	list = list->next;
		//}
	  
		//if (flag) {
		//	fr_pid_vpno_data *node = getmem(sizeof(fr_pid_vpno_data));
		//	node->pid = currpid;
		//	node->vpno = (virtpage + i);
		//	node->next = NULL;
		//	list->next = node;
		//}
	}
	
	//proctab[pid].backing_store_data[bs_id].bs_status = bsm_tab[bs_id].bs_status;		/* MAPPED or UNMAPPED		*/
	//proctab[pid].backing_store_data[bs_id].bs_pid = bsm_tab[bs_id].bs_pid;				/* process id using this slot   */
	//proctab[pid].backing_store_data[bs_id].bs_vpno = bsm_tab[bs_id].bs_vpno;			/* starting virtual page number */
	//proctab[pid].backing_store_data[bs_id].bs_npages = bsm_tab[bs_id].bs_npages;		/* number of pages in the store */
	//proctab[pid].backing_store_data[bs_id].bs_sem = bsm_tab[bs_id].bs_sem;
	//proctab[pid].backing_store_data[bs_id].bs_private_status = PRIVATE;					/* private access status */
	//proctab[pid].backing_store_data[bs_id].bs_ref_count = bsm_tab[bs_id].bs_ref_count;
	
	// Address Resolution for the creating process
	write_cr3(proctab[pid].pdbr);
	int temp_pid = currpid;
	currpid = pid;
	
	proctab[pid].vhpno = 4096;
	proctab[pid].vhpnpages = hsize;
	// Allocating memory for given hsize pages allowed for the 
	// process
	proctab[pid].vmemlist = getmem(sizeof(struct mblock));
	proctab[pid].vmemlist->mlen = 0;
	proctab[pid].vmemlist->mnext = (struct mblock*) (4096 * NBPG);
	proctab[pid].vmemlist->mnext->mlen = hsize * NBPG;
	proctab[pid].vmemlist->mnext->mnext = NULL;
	
	// Address resolution back to the current process
	currpid = temp_pid;
	write_cr3(proctab[currpid].pdbr);
	
	restore(ps);	
	return pid;
}

/*------------------------------------------------------------------------
 * newpid  --  obtain a new (free) process id
 *------------------------------------------------------------------------
 */
LOCAL	newpid()
{
	int	pid;			/* process id to return		*/
	int	i;

	for (i=0 ; i<NPROC ; i++) {	/* check all NPROC slots	*/
		if ( (pid=nextproc--) <= 0)
			nextproc = NPROC-1;
		if (proctab[pid].pstate == PRFREE)
			return(pid);
	}
	return(SYSERR);
}

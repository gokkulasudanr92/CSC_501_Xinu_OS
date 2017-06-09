#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

bs_map_t bsm_tab[NUM_OF_BACKING_STORE];
int get_bs(bsd_t bs_id, unsigned int npages) {
	STATWORD ps;
	disable(ps);
	
	// check for the parameters
	if (bs_id < 0 || bs_id > 7 || npages < 1 || npages > 256) {
		kprintf("Invalid parameters for the get_bs call.\n");
		restore(ps);
		return SYSERR;
	}
	
	if (bsm_tab[bs_id].bs_status == BSM_MAPPED && bsm_tab[bs_id].bs_private_status == PRIVATE) {
		kprintf("Trying to access a private heap of a process.\n");
		restore(ps);
		return SYSERR;
	}
	
	// Defining functionality as provided in the PA description
	if (bsm_tab[bs_id].bs_status == BSM_UNMAPPED) {
		bsm_tab[bs_id].bs_status = BSM_MAPPED;
		bsm_tab[bs_id].bs_npages = npages;
		bsm_tab[bs_id].bs_private_status = NON_PRIVATE;
		
		restore(ps);
		return npages;
	}
	
	restore(ps);
	return bsm_tab[bs_id].bs_npages;
}



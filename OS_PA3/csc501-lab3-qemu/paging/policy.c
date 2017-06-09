/* policy.c = srpolicy*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>


extern int page_replace_policy;
extern int debuging_enabled;
/*-------------------------------------------------------------------------
 * srpolicy - set page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL srpolicy(int policy)
{
  /* sanity check ! */
  STATWORD ps;
  disable(ps);
  
  page_replace_policy = policy;
  debuging_enabled = 1;
  if (page_replace_policy != SC || 
  page_replace_policy != AGING) {
	restore(ps);
	return SYSERR;
  }
  
  restore(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * grpolicy - get page replace policy 
 *-------------------------------------------------------------------------
 */
SYSCALL grpolicy()
{
  return page_replace_policy;
}

SYSCALL get_debug() {
	STATWORD ps;
	disable(ps);
	
	restore(ps);
	return debuging_enabled;
}


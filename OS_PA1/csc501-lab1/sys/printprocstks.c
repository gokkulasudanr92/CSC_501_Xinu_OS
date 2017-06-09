#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lab0.h>

unsigned long *esp;

void printprocstks(int priority) {
	kprintf("void printprocstks(int priority)\n");
	struct pentry *process;
	int i;
	for(i = 0; i < NPROC; i++) {
		process = &proctab[i];
		if (process->pstate != PRFREE && process->pprio > priority) {
			kprintf("Process [%s]\n", process->pname);
			kprintf("\tpid: %d\n", i);
			kprintf("\tpriority: %d\n", process->pprio);
			kprintf("\tbase: 0x%08x\n", process->pbase);
			kprintf("\tlimit: 0x%08x\n", process->plimit);
			kprintf("\tlen: %d\n", process->pstklen);
			if (process->pstate == PRCURR) {
				/* For the current process the proctab entry is out of date*/
				asm("movl %esp, esp");
				kprintf("\tpointer: 0x %08x\n", esp);	
			} else {
				kprintf("\tpointer: 0x %08x\n", process->pesp);	
			}
		}
	}
}

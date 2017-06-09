#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lab0.h>

/* Initialization of system call names */
char *syscalls[27] = {"sys_freemem", "sys_chprio", "sys_getpid", "sys_getprio", "sys_gettime", "sys_kill", "sys_receive", "sys_recvclr", "sys_recvtim", 
						"sys_resume", "sys_scount", "sys_sdelete", "sys_send", "sys_setdev", "sys_setnok", "sys_screate", "sys_signal", "sys_signaln", 
						"sys_sleep", "sys_sleep10", "sys_sleep100", "sys_sleep1000", "sys_reset", "sys_stacktrace", "sys_suspend", "sys_unsleep", "sys_wait"};
int is_tracked;

void syscallsummary_start() {
	is_tracked = TRUE;
}

void syscallsummary_stop() {
	is_tracked = FALSE;
}

void printsyscallsummary() {
	kprintf("void printsyscallsummary()\n");
	int i,j;
	for (i = 0; i < NPROC; i++) {
		struct pentry *process = &proctab[i];
		
		int check = 0;
			
		for (j = 0; j < NSYS; j++) {
			if (process->sysdata[j].count != 0) {
				check = 1;
				break;
			}
		}
			
		if (check == 1) {
			kprintf("Process [pid:%d]\n", i);
			
			for (j = 0; j < NSYS; j++) {
				if (process->sysdata[j].count != 0) {
					int avg = process->sysdata[j].total_execution_time / process->sysdata[j].count;
					kprintf("\tSyscall: %s, count: %d, average execution time: %ld (ms)\n", syscalls[j], process->sysdata[j].count, avg);
				}
			}
		}
	}
}

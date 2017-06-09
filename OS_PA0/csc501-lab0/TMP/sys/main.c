/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lab0.h>

int prX;
void halt();

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */
prch(c)
char c;
{
	int i;
	sleep(5);	
}

int main()
{
	kprintf("\n\nHello World, Xinu lives\n\n");
	//Check for little endian
	unsigned int x = 1;
	if ((int) (((char *)&x))) {
		kprintf("LITTLE ENDIAN\n");
	} else {
		kprintf("BIG ENDIAN\n");
	}
	// Program 1
	unsigned long input = 2864434397u;
	kprintf("\nThe input for zfunction: 0x%x\n", input);
	unsigned long result_zfunction = zfunction(input);
	kprintf("\nThe result of zfunction: 0x%x\n", result_zfunction);
	//Program 2
	kprintf("\n\nProgram 2: Print Segment Address\n\n");
	printsegaddress();
	// Program 3
	kprintf("\n\nProgram 3: Print Top of Stack\n\n");
	printtos();
	//Program 4
	kprintf("\n\nProgram 4: Print Process Stack\n\n");
	printprocstks(-1);
	//Program 5
	kprintf("\n\nProgram 5: Print System Call Summary\n\n");
	syscallsummary_start();
	resume(prX = create(prch,2000,20,"proc X",1,'A'));
	sleep(10);
	syscallsummary_stop();
	printsyscallsummary();
	return 0;
}

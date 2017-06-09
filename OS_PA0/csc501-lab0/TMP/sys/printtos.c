#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <lab0.h>

unsigned long *esp;
unsigned long *ebp;

void printtos() {
	kprintf("void printtos()\n");
	unsigned long *sp, *fp;
	asm("movl %ebp, ebp");
	fp = ebp;
	asm("movl %esp, esp");
	sp = esp;
	kprintf("Before [0x%08x]: 0x%08x\n", (fp + 1), *(fp + 1));
	kprintf("After [0x%08x]: 0x%08x\n", fp, *fp);
	long i;
	for (i = 0; i < 4 && sp < fp; i++, sp++) {
		kprintf("\telement[0x%08x]: 0x%08x\n", sp, *sp);
	}
}

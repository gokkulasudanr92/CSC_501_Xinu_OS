/* zfunction.s - _zfunction */

		.text
		.globl	zfunction
zfunction:
		cli
		pushl %ebp				/* Get the pointer to data on stack */
		movl %esp, %ebp			/* Move the stack pointer to stack data pointer */
		movl 8(%esp), %eax		/* To get the argument of parameter and moving the value to %eax register */
		
		andl $0xff803fff, %eax	/* To clear the 10th bits through 18th bits */
		sall $4, %eax			/* Shift Arithmetic left for 4 bits */
		leave
		ret
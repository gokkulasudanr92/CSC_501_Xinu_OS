# PA 1: Process Scheduling

1. Objective

The objective of this assignment is to get familiar with the concepts of process management, including process priorities, scheduling, and context switching.

2. Readings

The Xinu source code for functions in sys/, especially those related to process creation (create.c), scheduling (resched.c, resume.c suspend.c), termination (kill.c), changing priority (chprio.c), system initialization (initialize.c), and other related utilities (e.g., ready.c), etc.

3. What To Do

You will also be using the csc501-lab0.tgz. You have downloaded and compiled it in PA 0, but this time you need to rename the whole directory to csc501-lab1.

In this assignment, you will implement two new scheduling policies, which avoid the starvation problem in process scheduling. At the end of this assignment, you will be able to explain the advantages and disadvantages of the two new scheduling policies.

The default scheduler in Xinu schedules processes based on the highest priority. Starvation occurs when two or more processes that are eligible for execution have different priorities. The process with the higher priority gets to execute first, resulting in processes with lower priorities never getting any CPU time unless process with the higher priority ends.

The two scheduling policies that you need to implement, as described as follows, should address this problem. Note that for each of them, you need to consider how to handle the NULL process, so that this process is selected to run when and only when there are no other ready processes.

For Linux-like scheduling policies, the value of a valid process priority is an integer between 0 to 99, where 99 is the highest priority.

1) Exponential Distribution Scheduler

The first scheduling policy is the exponential distribution scheduler. This scheduler chooses the next process based on a random value that follows the exponential distribution. When a rescheduling occurs, the scheduler generates a random number with the exponential distribution, and chooses a process with the lowest priority that is greater than the random number. If the random value is less than the lowest priority in the ready queue, the process with the lowest priority is chosen. If the random value is no less than the highest priority in the ready queue, the process with the largest priority is chosen. When there are processes having the same priority, they should be scheduled in a round-robin way. (HINT: The moment you insert a process into the ready queue, the queue is ordered.)

For example, let us assume that the scheduler uses a random value with the exponential distribution of λ = 0.1, and there are three processes A, B, and C, whose priorities are 10, 20, and 30, respectively. When rescheduling happens, if a random value is less than 10, process A will be chosen by the scheduler. If the random value is between 10 and 20, the process B will be chosen. If the random value is no less than 20, process C will be chosen. The probability that a process is chosen by the scheduler follows the exponential distribution. As shown in the figure below, the ratio of processes A (priority 10), B (priority 20), and C (priority 30) to be chosen is 0.63 : 0.23 : 0.14. It can be mathematically calculated by the cumulative distribution function F(x; λ) = 1 - e-λx.


In order to implement an exponential distribution scheduler, you need to implement expdev(), which generates random numbers that follow the exponential distribution. The generator can be implemented by -1/λ * log(1 - y), where y is a random number following the uniform distribution in [0, 1].

Implementation:

double expdev(double lambda) {
    double dummy;
    do
        dummy= (double) rand() / RAND_MAX;
    while (dummy == 0.0);
    return -log(dummy) / lambda;
}
	
Implementation Tips: Since Xinu does not provide any math library (i.e., math.h), you will implement log() by yourself. For log() implementation, you can use Taylor series (http://en.wikipedia.org/wiki/Taylor_series), where n = 20 can give an reasonable approximation. Note that Xinu does not offer any format to print out a floating number, so you can first test your code with gcc in Linux and use it in Xinu. You can also roughly test it in Xinu by casting, e.g. kprintf("%d", (int) log(20)). You will also have to implement pow(). You are recommended to put expdev(), log(), and pow() in math.c and add it to Makefile. On the other hand, srand() and rand() functions are offered in lib/libxc/rand.c by Xinu.

double log(double x);
double pow(double x, int y);
double expdev(double lambda);
Comment: As the value of λ, use 0.1. The mean of the exponential distribution is 1/λ, so it is 10 for λ = 0.1. Since the priority ranges from 0 to 99, 10 can be meaningful. The λ value may be changed for a different scheduling property, but your code will be tested for λ = 0.1.

2) Linux-like Scheduler (based loosely on the Linux kernel 2.2 )

This scheduling algorithm loosely emulates the Linux scheduler in the 2.2 kernel. We consider all the processes "conventional processes" and use the policies of the SCHED_OTHER scheduling class within the 2.2 kernel. With this algorithm, the CPU time is divided into epochs. In each epoch, every process has a specified time quantum, whose duration is computed at the beginning of the epoch. An epoch ends when all the runnable processes have used up their quantum. If a process has used up its quantum, it will not be scheduled until the next epoch starts. A process can be scheduled many times during an epoch if it has not used up its quantum.

When a new epoch starts, the scheduler recalculates the time quantum of all processes (including the blocked ones). This way, a blocked process can start in the new epoch when it becomes runnable again. New processes created in the middle of an epoch have to wait until the next epoch. For a process that has never executed or has exhausted its time quantum in the previous epoch, its new quantum value is set to its process priority (i.e., quantum = priority). A quantum of 10 allows a process to execute for 10 ticks (10 timer interrupts) within an epoch. For a process that has not used up its previously assigned quantum, we allow a part of the unused quantum to be carried over to the new epoch. Suppose for each process, a variable counter describes how many ticks are left from its previous quantum, then at the beginning of the next epoch, quantum = floor(counter / 2) + priority. For example, a counter of 5 and a priority of 10 produce a new quantum value of 12.
During each epoch, runnable processes are scheduled according to their goodness. For processes that have used up their quantum, their goodness value is 0. For other runnable processes, their goodness value is set considering both their priority and the amount of quantum allocation left: goodness = counter + priority. Again, round-robin is used among processes with the equal goodness.

The priority of a process can be changed explicitly during the system call create() or through the function chprio(). Priority changes made in the middle of an epoch only take effect in the next epoch.

An example of how processes should be scheduled under this scheduler is as follows:

If there are processes P1, P2, P3 with priority 10, 20, 15, the epoch would be equal to 10 + 20 + 15 = 45, and the possible schedule (with quantum duration specified in the braces) could be: P2 (20), P3 (15), P1(10), P2(20) ,P3(15), P1(10), but not: P2 (20), P3 (15), P2 (20), P1 (10).

3) Other Implementation Details

1. void setschedclass (int sched_class) 
      This function should change the scheduling type to either EXPDISTSCHED or LINUXSCHED. 

2. int getschedclass() 
      This function should return the scheduling class, which should be either EXPDISTSCHED or LINUXSCHED. 

3. Each of the scheduling class should be defined as a constant: 
      #define EXPDISTSCHED 1 
      #define LINUXSCHED 2 

4. Some of the source files of interest are: create.c, resched.c, resume.c, suspend.c, ready.c, proc.h, kernel.h, etc. 

4) Test Cases

You should use testmain.c as your main.c program. 

For Exponential Distribution Scheduler: 
Three processes A, B, and C are created with priorities 10, 20, 30. In each process, we keep increasing a variable, so the variable value is proportional to CPU time. Note that the ratio should be close to 0.63 : 0.23 : 0.14, as the three processes are scheduled based on the exponential distribution. You are expected to get similar results as follows: 

Start... A
Start... B
Start... C

Test Result: A = 982590, B = 370179, C = 206867 

For Linux-like Scheduler: 
Three processes A,B and C are created with priorities 5, 50 and 90. In each process, we print a character ('A', 'B', or 'C') at a certain time interval for LOOP times (LOOP = 50). The main process also prints a character ('M') at the same time interval. You are expected to get similar results as follows: 

MCCCCCCCCCCCCCBBBBBBBBMMMACCCCCCCCCCCCCB
BBBBBMMMACCCCCCCCCCCCBBBBBBBMMMACCCCCCCC
CCCCBBBBBBBMMMBBBBBBBMMABBBBBBBMMMABBBBB
BBMMMBMMMAMMAMMMAMMMMMMAMMMAMMMMMAMMMAMM
MMAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

4. Concurrency Programming

NOTE: This is not Xinu, but Linux programming. Your code will be tested on the EOS system, so make sure that your code properly works on it.

Develop a program using POSIX threads to find the number of occurrences of an input search string in a given new-line delimited source. Your program needs to operate as follows:

string_search "searchString" < inputFile
The two parameters are an input file with the source and the string to search the source for. You can assume search strings will be no longer than 80 characters. Searches should be parallelized to threads where each thread operates on a different data block (not necessarily terminated by a newline).The number of matches per block (partial result of a thread) may be stored in a global array that is aggregated in the main thread (after joining with other threads) to produce a final result number. Plot your running time while using different numbers of threads and block sizes in order to find an optimal run configuration on a Linux machine. Submit the plot as a jpg file.

Hints: use the following man pages: pthread_create, pthread_join, make, gcc.

Search strings may extend past data block boundaries, you must account for this. For example consider searching for "foo" in the following:

----------Data block handled by Thread 1 ---------------
This is some test text with the word foo\n
two times. Once at the beginning and fo


----------Data block handled by Thread 2 ---------------
o again there.\n
In this case our search string, "foo", starts on line 2 and ends on line 3.

Example: $ ./string_search "This is" < sample.html > string_search.out 
Input: sample.html 
Output: string_search.out string_search.jpg 
Turn in files string_search.c, string_search.jpg and Makefile

5. Additional Questions

Write your answers to the following questions in a file named Lab1Answers.txt(in simple text). Please place this file in the sys/ directory and turn it in, along with the above programming assignment.

What are the advantages and disadvantages of each of the two scheduling policies? Also, give the advantages and disadvantages of the round robin scheduling policy originally implemented in Xinu.
Describe the way each of the schedulers affects the NULL process.

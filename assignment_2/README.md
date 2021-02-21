# SECOND ASSIGNMENT 
## Assignment 2

The second assignment implements the struct of a timer. The timer is a periodic object which in each period performs a specific task. In this assignment the timer adds functions to a queue. These functions will later be executed from consumers threads. 
As each timer has a different period and insertions are asynchronous the implementation uses threads. 

Since power consumption is key to the implementation of the assignment to run in a raspberry pi zero, few optimization techniques have been performed.

### How to run:

**1.** Use the following commands to compile the code in PC : gcc -O2 src/timer.c src/queue.c main.c -o <executable_name> -lm -lpthread

**2.** Use the following commands to compile the code for raspberry pi zero (ARM architecture). Download cross compilers from the link(https://sourceforge.net/projects/raspberry-pi-cross-compilers/files/). Use the command arm-linux-gnueabihf-gcc -g -o <executable_name> src/timer.c src/queue.c main.c -lpthread -lm

**3.** Use the command scp to copy the executable to raspberry pi and then connect to it and run the executable.

# Master-worker project

In this group project, we were asked to program the conversion of a binary number to its decimal equivalent using message queues, shared memory, semaphores and signals. 

Master (server):
1) Creates message queue, attaches to shared memory and initializes semaphore for worker.
2) Creates matrix for binary number (size of matrix = number of workers).
3) Receives binary number from workers (filled matrix) and converts it to decimal.

Worker (client):
1) Attaches to shared memory and receive semaphore. 
2) Each worker has a process id which corresponds to its designated cell in the matrix, and it's randomly assigned a value of 0 or 1 (binary digit).

--------

This repository contains:
- Master code (.c file)
- Worker code (.c file)

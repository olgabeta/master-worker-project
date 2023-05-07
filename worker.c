/*
    Code written by:
    Stefania Douliaka, 00974
    Panagiotis Karoutsos, 02034
    Olga Vasileiou, 01691
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>


struct msg {                    // struct for the messages in the Message Queue 
    long mtype;
    int shmkey;
    int semid;
};

int main(int argc, char* argv[]){

    int i, shmid, size, task_id, *matrix, msgqid, pid, semid, *sharedMemory, power, result, msgqkey, shmkey;
    struct msg message;
    struct sembuf op;                   

    msgqkey = ftok(".",100);                    // generate a key based on the 100th character of the current file path for the message queue

    msgqid = msgget(msgqkey,0666|IPC_CREAT);    // msgget creates a message queue and returns identifier

    printf("Waiting for the master program to be executed.\n");

    
    while((int)msgrcv(msgqid,&message,sizeof(message),1,0) < 0){    // msgrcv to receive message from the Message Queue
        // wait for the master program to be executed
    }

    shmkey = message.shmkey;
    semid = message.semid;

    printf("Waiting for the master program to fill the shared memory.\n");
    op.sem_num = 0;
    op.sem_op = 0;
    op.sem_flg = 0;
    semop(semid,&op,1);                         // set value of semaphore 0 to 0 to block workers

    shmid = shmget(shmkey,(3+(2*size))*sizeof(int),0666|IPC_CREAT);     // shmget creates a shared memory and returns identifier
    sharedMemory = (int*)shmat(shmid,NULL,0);                           // attach to shared memory
    pid = sharedMemory[0];
    task_id = sharedMemory[1];
    size = sharedMemory[2];

    op.sem_num = 1;
    op.sem_op = -1;
    op.sem_flg = IPC_NOWAIT;
    semop(semid,&op,1);                         // decrease the value of semaphore 1 in order to block other workers
    
    task_id = sharedMemory[1];
    sharedMemory[1]++;                          // is used by the next worker

    printf("\nTask ID is: %d\n", task_id);

    op.sem_num = 1;
    op.sem_op =  1;
    op.sem_flg = 0;
    semop(semid,&op,1);                         // increase the value of semaphore 1 by 1 in order to unblock the other workers

    matrix = (int*)malloc(size*sizeof(int));

    for(i = 0; i < size; i++){
       matrix[i] = sharedMemory[3+i];
    }
    
    if (task_id == size-1){       
        op.sem_num = 2;
        op.sem_op = size;
        op.sem_flg = 0;
        semop(semid,&op,1);                     // increase the value of semaphore 2 by size in order to unblock the workers
    }
    else{
        printf("\nWaiting...\nYou need to run %d more task(s) to proceed.\n\n", size-(task_id+1));
        op.sem_num = 2;
        op.sem_op = -1;
        op.sem_flg = 0;
        semop(semid,&op,1);                     // decrease the value of semaphore 2 by 1 in order to block until all workers are ready
    }
    
    printf("Binary value of cell[%d]: %d\n", task_id, matrix[task_id]);

    power = 1;
    for(i = 0; i < size - task_id - 1; i++){
        power *= 2;                             // calculate the power of 2 depending on the cell of the matrix
    }
    
    result = matrix[task_id] * power;           // decimal result of each cell of the matrix

    sharedMemory[3+size+task_id] = result;      // write the result in shared memory
    
    printf("Result of cell[%d]: %d\n", task_id, result);
    
    sleep(1+task_id);
    kill(pid,SIGUSR1);                          // send signal to the process of the master.c executable
    
    shmdt(sharedMemory);                        // detach from shared memory
    msgctl(msgqid,IPC_RMID,NULL);
    shmctl(shmid,IPC_RMID,NULL);
    semctl(semid,0,IPC_RMID);
    semctl(semid,1,IPC_RMID);
    semctl(semid,2,IPC_RMID);                   // destroy message queue, shared memory and all created semaphores
    free(matrix);                               // free allocated memory for matrix
    return(0);
}

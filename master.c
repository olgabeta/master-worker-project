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
#include <sys/stat.h>
#include <time.h>

#define _GNU_SOURCE


int msgqid, *sharedMemory, shmid, semid;    // initialize global variables

static volatile sig_atomic_t counter = 0;   // sig_atomic_t data type guarantees communication with main process and handler function

struct msg {                                // struct for the messages in the Message Queue 
    long mtype;
    int shmkey;
    int semid;
};

// declaration of functions
void handler(int sig);
void fill_matrix(int *matrix, int size);
int matrix_sum(int *results, int size);
void signal_handler(int);

int main(int argc, char* argv[]){

    int i, size, *matrix, msgqkey, shmkey, pid, *results, decimal_number;
    struct sigaction act = {{0}};
    struct msg message;

    if(argc != 2){
        printf("Write '%s number' instead, where number = amount of binary bits to be translated.\n", argv[0]);
        exit(0);
    }

    signal(SIGINT,signal_handler);      // catch ctrl+c generation signal (SIGINT) and call signal_handler function when it happens

    size = atoi(argv[1]);
    matrix = (int*)malloc(size*sizeof(int));
    results = (int*)malloc(size*sizeof(int));
    sharedMemory = (int*)malloc((3+(2*size))*sizeof(int));  // allocate matrix, results and sharedMemory according to user input

    msgqkey = ftok(".",100);                    // generate a key based on the 100th character of the current file path for the message queue

    printf("Message Queue key: %d\n",msgqkey);

    msgqid = msgget(msgqkey,0666|IPC_CREAT);    // msgget creates a message queue and returns identifier
    message.mtype = 1;

    srand(time(NULL));
    do{
        shmkey = ftok(".",(int)rand());         // generate a key based on a random character of the current file path for the shared memory
    }while(shmkey < 0);                         

    printf("Shared Memory key: %d\n",shmkey);

    shmid = shmget(shmkey,(3+(2*size))*sizeof(int),0666|IPC_CREAT);     // shmget creates a shared memory and returns identifier
    semid = semget(IPC_PRIVATE,3,S_IRWXU);                              // semget creates a semaphore and returns identifier
    semctl(semid,0,SETVAL,0);                                           // set value of semaphore 0 to 0

    message.shmkey = shmkey;
    message.semid = semid;

    for(i = 0; i < size; i++){
        msgsnd(msgqid,&message,sizeof(message),0);          // msgsnd to send message to the Message Queue
        // send to queue as many messages as the required workers for the needed task to be done
    }

    semctl(semid,0,SETVAL,2);                       // set value of semaphore 0 to 2 to make the workers wait until shared memory is filled

    act.sa_handler = handler;
    sigaction(SIGUSR1, &act, NULL);                 // assign signal SIGUSR1 to handler function

    pid = getpid();                                 // get current process id (master pid)

    fill_matrix(matrix,size);
    
    sharedMemory = (int*)shmat(shmid,NULL,0);       // attach to shared memory
    sharedMemory[0] = pid;
    sharedMemory[1] = 0;                            // sharedMemory[1] is used by the program for the workers
    sharedMemory[2] = size;

    for(i = 0; i < size; i++){
        sharedMemory[3+i] = matrix[i];
    }

    semctl(semid,0,SETVAL,0);                       // set value of semaphore 0 to 0 to let the workers continue their functioning

    printf("\nWaiting...\nYou need to run %d task(s) to proceed.\n", size);

    while(counter < size){                
        //waiting until all tasks are complete
    }

    printf("\nThe binary number was: ");            // print array of binary bits
    for(i = 0; i < size; i++){
        printf("%d", matrix[i]);
    }

    for(i = 0; i < size; i++){
       results[i] = sharedMemory[3+size+i];
    }

    decimal_number = matrix_sum(results, size);
    printf("\nThe equivalent decimal number is: %d\n",decimal_number); 
    
    shmdt(sharedMemory);                    // detach from shared memory
    msgctl(msgqid,IPC_RMID,NULL);
    shmctl(shmid,IPC_RMID,NULL);
    semctl(semid,0,IPC_RMID);
    semctl(semid,1,IPC_RMID);
    semctl(semid,2,IPC_RMID);               // destroy message queue, shared memory and all created semaphores
    free(matrix);                           
    free(results);                          // free allocated memory for arrays
    return(0);
}


// definition of functions

void handler(int sig){
    counter++;
}

void fill_matrix(int *matrix, int size){

    int i, random;

    srand(time(NULL));
    
    for (i = 0; i < size; i++){             // fill array with random binary integers (0 or 1)

        random = rand()%2 + 1;
        matrix[i] = random - 1;
    }     
}

int matrix_sum(int *results, int size){     // sum of all matrix elements

    int i, decimal_number = 0;

    for(i = 0; i < size; i++){
        decimal_number += results[i];
    }

    return decimal_number;
}

void signal_handler(int signum){

    shmdt(sharedMemory);                    // detach from shared memory
    msgctl(msgqid,IPC_RMID,NULL);           // destroy message queue, shared memory and semaphores
    shmctl(shmid,IPC_RMID,NULL);
    semctl(semid,0,IPC_RMID);
    semctl(semid,1,IPC_RMID);
    semctl(semid,2,IPC_RMID);           
    exit(0);                                // close the program
}

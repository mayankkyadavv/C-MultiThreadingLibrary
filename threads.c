#include <pthread.h>
#include "threads.h"
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>

#define maxThreads 128
#define stackSize 32767
#define interval (50000)
#define ALIGNMENT 16

enum state{
    RUNNING,
    READY,
    EXITED,
    EMPTY,
    BLOCKED
};

struct TCB{
    pthread_t tid;
    int* stack;
    jmp_buf registers;
    enum state status;
    pthread_t join_target;
    void* return_value;
};

static struct TCB TCB_TABLE[maxThreads];
static struct sigaction signal_handler;
static pthread_t runningTid = 0;
static sigset_t mask;

struct my_semaphore {
    int value;
    int initialized;
    pthread_t waiting_threads[maxThreads];
    int front, rear;
};
typedef struct my_semaphore my_sem_t;



void lock() {
    sigemptyset(&mask);
    sigaddset(&mask, SIGALRM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

void unlock() {
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

static void schedule(int signo){
    if (signo == SIGALRM) {
        //printf("Alarm signal received\n");
    }

    if(TCB_TABLE[runningTid].status == RUNNING){
        TCB_TABLE[runningTid].status = READY;
    }

    pthread_t scheduleTid = runningTid;
    while(1){
        if(scheduleTid == maxThreads - 1){
            scheduleTid = 0;
        }
        else{
            scheduleTid++;
        }

        if(TCB_TABLE[scheduleTid].status == READY){
            break;
        }
    }

    int jump = 0;
    if(TCB_TABLE[runningTid].status != EXITED){
        jump = setjmp(TCB_TABLE[runningTid].registers);
    }

    if(jump == 0){
        TCB_TABLE[scheduleTid].status = RUNNING;
        runningTid = scheduleTid;
        longjmp(TCB_TABLE[runningTid].registers, 1);
    }
}

int sem_init(sem_t *sem, int pshared, unsigned value) {
    if (pshared != 0) {
        return -1;
    }

    my_sem_t *my_sem = (my_sem_t *) malloc(sizeof(my_sem_t));
    if (my_sem == NULL) {
        return -1;
    }

    my_sem->value = value;
    my_sem->initialized = 1;
    my_sem->front = my_sem->rear = 0;
    for (int i = 0; i < maxThreads; i++) {
        my_sem->waiting_threads[i] = -1;
    }

    sem->__align = (long) my_sem;
    return 0;
}

int sem_wait(sem_t *sem) {
    my_sem_t *my_sem = (my_sem_t *) sem->__align;
    if (!my_sem->initialized) {
        return -1;
    }

    lock();

    while (my_sem->value <= 0) {
        //add the current thread to the waiting queue
        my_sem->waiting_threads[my_sem->rear] = pthread_self();
        my_sem->rear = (my_sem->rear + 1) % maxThreads;

        //block the current thread
        TCB_TABLE[pthread_self()].status = BLOCKED;
        unlock();
        schedule(SIGALRM);
        lock();
    }

    my_sem->value--;
    unlock();
    return 0;
}

int sem_post(sem_t *sem) {
    my_sem_t *my_sem = (my_sem_t *) sem->__align;
    if (!my_sem->initialized) {
        return -1; //semaphore not initialized
    }

    lock(); 

    if (my_sem->waiting_threads[my_sem->front] != -1) {
        //wake up one waiting thread
        pthread_t tid = my_sem->waiting_threads[my_sem->front];
        my_sem->waiting_threads[my_sem->front] = -1;
        my_sem->front = (my_sem->front + 1) % maxThreads;

        TCB_TABLE[tid].status = READY;
    } else {
        my_sem->value++;
    }

    unlock();
    return 0;
}

int sem_destroy(sem_t *sem) {
    my_sem_t *my_sem = (my_sem_t *) sem->__align;
    if (!my_sem->initialized) {
        return -1; 
    }
    free(my_sem);
    sem->__align = 0;
    return 0;
}

static void setupTableAlarm(){
    for(int i = 0; i < maxThreads; i++){
        TCB_TABLE[i].tid = i;
        TCB_TABLE[i].status = EMPTY;
    }

	ualarm((useconds_t) interval, (useconds_t) interval);
	sigemptyset(&signal_handler.sa_mask);
	signal_handler.sa_handler = &schedule;
	signal_handler.sa_flags = SA_NODEFER;
	sigaction(SIGALRM, &signal_handler, NULL);
    TCB_TABLE[0].status = READY;
}


int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg){
    static bool firstCall = true;
    int firstThread = 0;
    attr = NULL;

    if(firstCall == true){
        firstCall = false;
        setupTableAlarm();
        firstThread = setjmp(TCB_TABLE[0].registers);
    }

    if(firstThread == 0){
        pthread_t current;
        bool reachedLimit = true;

        for(int i = 1; i < maxThreads; i++){
            if(TCB_TABLE[i].status == EMPTY){
                reachedLimit = false;
                current = i;
                break;
            }
        }
        
        if(reachedLimit == true){
            printf("Error: Thread limit has been reached\n");
            exit(EXIT_FAILURE);
        }
        
       *thread = current;
       TCB_TABLE[current].registers[0].__jmpbuf[7] = ptr_mangle((unsigned long int) start_thunk);
       TCB_TABLE[current].registers[0].__jmpbuf[3] = (long) arg;
       TCB_TABLE[current].registers[0].__jmpbuf[2] = (unsigned long int) start_routine;
       TCB_TABLE[current].stack = malloc(stackSize);
       if (TCB_TABLE[current].stack == NULL) {
           perror("Failed to allocate stack");
           exit(EXIT_FAILURE);
       }
       unsigned long aligned_stack_top = ((unsigned long)TCB_TABLE[current].stack + stackSize) & ~(ALIGNMENT - 1);
       aligned_stack_top -= 8;  // adjust stack pointer (8 bytes for a 64-bit address)
       *(unsigned long *)aligned_stack_top = (unsigned long)pthread_exit_wrapper;
       TCB_TABLE[current].registers[0].__jmpbuf[6] = ptr_mangle(aligned_stack_top);
       TCB_TABLE[current].status = READY;
       TCB_TABLE[current].tid = current;
    }   
    else{
        firstThread = 0;
    }
    
    return 0;
}


int pthread_join(pthread_t thread, void **value_ptr) {
    if (TCB_TABLE[thread].status == EXITED) {
        if (value_ptr != NULL) {
            *value_ptr = TCB_TABLE[thread].return_value;
        }
        return 0; 
    } else if (TCB_TABLE[thread].status == EMPTY) {
        return EINVAL; 
    } else {
        TCB_TABLE[runningTid].status = BLOCKED;
        TCB_TABLE[runningTid].join_target = thread;
        schedule(1234.4321);
        // After being unblocked and scheduled again:
        if (value_ptr != NULL) {
            *value_ptr = TCB_TABLE[thread].return_value;
        }
        return 0; 
    }
}

void pthread_exit(void *value_ptr){
    TCB_TABLE[runningTid].status = EXITED;
    TCB_TABLE[runningTid].return_value = value_ptr;

    for (int i = 0; i < maxThreads; i++) {
        if (TCB_TABLE[i].status == BLOCKED && TCB_TABLE[i].join_target == runningTid) {
            TCB_TABLE[i].status = READY;
        }
    }

    bool moreThreads = false;
    for(int i = 0; i < maxThreads; i++){
        if(TCB_TABLE[i].status == READY){
            moreThreads = true;
            break;
        }
    }

    if(moreThreads == true){
        schedule(1234.4321);
    }

    for(int i = 0; i < maxThreads; i++){
        if(TCB_TABLE[i].status == EXITED){
            free(TCB_TABLE[i].stack);
        }
    }
   
    exit(0);
}

pthread_t pthread_self(void){
    return runningTid;
}

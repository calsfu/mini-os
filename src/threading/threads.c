#include "thread_mangler.h"
#include <setjmp.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "threads.h"

#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6
#define JB_PC 7
#define MAXTHREADS 129

void schedule();

//static int inThread = 0;

static struct sigaction sa; 
static sigset_t set;

static int gCurrent = 0; //id of currently running thread
static int threadCount = 0;
static int locked = 0;

//static int scheduleStarted = 0;
struct thread { //TCB
    int id; //thread id
    jmp_buf buf;  //set of registers
    size_t* sp; //stack pointer
    enum {READY, RUNNING, EXITED, BLOCKED } status; //status
    void* exit_status; //return value
};

struct MySem {
    sem_t sem;
    int intialized;
    int value;
    int* queue;
};

static int nextQTake = 0;
static int nextQAdd = 0;

static struct thread TCBArr[MAXTHREADS]; //array of threads

static void helper() {
    
    sa.sa_flags = SA_NODEFER;
    sa.sa_handler = &schedule;
    sigaction(SIGALRM, &sa, NULL);
    //scheduleStarted = 1;
    //perror("sigaction");
    ualarm(50000, 50000);

   
    // sigaction(SIGALRM, &sa, *schedule);
    // ualarm(50000);
    // schedule();
    //printf("0x%08lx\n", ptr_demangle(TCBArr[0].buf[0].__jmpbuf[JB_PC]));
}

int pthread_create(
    pthread_t *thread,
    const pthread_attr_t *attr,
    void *(*start_routine) (void *),
    void *arg
) 
{
    /*
    Arguments
    thread: pointer to new thread id
    attr: NULL
    start_routine: address to where thrad should start. should go in R12, then use start_thunk
    arg: argument to pass to start_routine. should go in R13, then use start_thunk
    
    1. Cerate a new TCB
    2. Set state to ready
    3. Call schedule

    Empty stack: rip, rbp, then rest of program
    */
    //setjmp(buf);
    
    //create main thread
    if(threadCount == 0) {
        struct thread *mainThread = malloc(sizeof(struct thread));
        mainThread->id = threadCount;
        mainThread->status = RUNNING;
        size_t* stack = (size_t*) malloc(32767);
        // printf("beggining of stack: 0x%08lx\n", (long unsigned int) stack);
        // printf("end of stack: 0x%08lx\n", (long unsigned int) stack + 32767);
        // printf("size of pthread_exit: %lu\n", sizeof(pthread_exit));
        //*(stack - 5) = (size_t) &pthread_exit;
        mainThread->sp = stack;
        mainThread->exit_status = NULL;
        mainThread->buf[0].__jmpbuf[JB_RSP] = ptr_mangle((long unsigned int)stack);
        *(stack) = (size_t) &pthread_exit_wrapper;
        TCBArr[threadCount] = *mainThread;
        threadCount++;
    }

    size_t* stack = (size_t*) malloc(32767);
    // void* routineptr = ptr_mangle(*start_routine);
    // void* argptr = ptr_mangle(arg);
    struct thread *newThread = malloc(sizeof(struct thread));
    newThread->id = threadCount;
    newThread->status = READY;
    newThread->sp = stack;
    newThread->exit_status = NULL;
    //only rsp and rip need to be mangled
    newThread->buf[0].__jmpbuf[JB_PC] = ptr_mangle((long unsigned int)&start_thunk);
    newThread->buf[0].__jmpbuf[JB_R12] = (long unsigned int)start_routine; //PC: pointer to start thunk
    newThread->buf[0].__jmpbuf[JB_R13] = (long unsigned int)arg; //RDI? : argument to start thunk
    newThread->buf[0].__jmpbuf[JB_RSP] = ptr_mangle((long unsigned int)stack);
    //printf("start thunk: 0x%08lx\n", &start_thunk);
    // printf("R12: 0x%08lx\n", buf[0].__jmpbuf[JB_R12]);
    // printf("R13: 0x%08lx\n", buf[0].__jmpbuf[JB_R13]);
    
    //set beginning of stack pointer to pthread_exit
    //put address of pthread exit to top of stack
    *(stack) = (size_t) &pthread_exit_wrapper;
    TCBArr[threadCount] = *newThread; //add thread to array

    *thread = (pthread_t) threadCount; //give thread id to thread
    //printf("function size %lu\n", sizeof(**start_routine));
    
    threadCount++;
    
    if(threadCount == 2) {
        helper();
    }
    return 1;
}


void pthread_exit(void *value_ptr) {
    TCBArr[gCurrent].status = EXITED;
    TCBArr[gCurrent].exit_status = value_ptr;
    schedule();
    free(TCBArr[gCurrent].sp);
    free(&TCBArr[gCurrent]);
    //get return value of function to return
    exit(0);
}

pthread_t pthread_self(void) {
    return gCurrent;
}

int pthread_join(pthread_t thread, void **value_ptr) {
    if(thread < 0 || thread >= threadCount) return -1;
    TCBArr[gCurrent].status = BLOCKED;
    if(TCBArr[thread].status == EXITED) {
        if(value_ptr != NULL) {
            *value_ptr = TCBArr[thread].exit_status;
        }
        TCBArr[gCurrent].status  = READY;
        return 0;
    }
    else {
        TCBArr[gCurrent].status = BLOCKED;
        while(TCBArr[thread].status != EXITED && TCBArr[thread].exit_status == NULL) {
            schedule();
        }
        if(value_ptr != NULL) {
            *value_ptr = TCBArr[thread].exit_status;
        }
        TCBArr[gCurrent].status = READY;
        return 0;
    }
}


void schedule() {
    //alarm every 50 ms
    //setjmp to save state
    //longjmp to restore state 
    // if(scheduleStarted) {
    //     printf("schedule\n");
    //     scheduleStarted = 0;
    //     longjmp(TCBArr[gCurrent].buf, 1); //context switch to next thread
    // }
    
    int prev = gCurrent;
    gCurrent = (gCurrent + 1) % threadCount;
    if(TCBArr[prev].status == RUNNING) { //check if running
        TCBArr[prev].status = READY;
    }
    // //printf("gCurrent: %d\n", gCurrent);
    while(TCBArr[gCurrent].status == EXITED) { //
        if(gCurrent == prev && TCBArr[gCurrent].status == EXITED) {
            exit(0);
        }
        gCurrent = (gCurrent + 1) % threadCount;
    }

    //check if thread function returned
    //if so, run pthread_exit
    //if not, run next thread

    TCBArr[gCurrent].status = RUNNING;
    // siglongjmp(TCBArr[gCurrent].buf, 1);//context switch to next thread
    if (setjmp(TCBArr[prev].buf) == 0) { //save state of current thread
        longjmp(TCBArr[gCurrent].buf, 1); //context switch to next thread
    }

}

void lock() {
    //block alarm for current thread
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigprocmask(SIG_BLOCK, &set, NULL);
    locked = 1;
}

void unlock() {
    //unblock alarm for curretn thread
    locked = 0;
    sigprocmask(SIG_UNBLOCK, &set, NULL);   
}

void pthread_exit_wrapper()
{
    unsigned long int res;
    asm("movq %%rax, %0\n":"=r"(res));
    pthread_exit((void *) res);
}

int sem_init(sem_t *sem, int pshared, unsigned value) {
    if(value < 0) return -1;
    lock();
    struct MySem* newSem = malloc(sizeof(struct MySem));
    newSem->value = value;
    newSem->intialized = 1;
    newSem->queue = malloc(sizeof(int) * MAXTHREADS);
    sem->__align = (long int) newSem;
    unlock();
    return 0;
}
int sem_wait(sem_t *sem) {
    struct MySem *sem2 = (struct MySem*) sem->__align;
    lock();
    if(sem2 -> value > 0) {
        sem2 -> value--;
    }
    else {
        TCBArr[gCurrent].status = BLOCKED;
        sem2->queue[nextQAdd] = gCurrent;
        nextQAdd = (nextQAdd + 1) % MAXTHREADS;
        while(TCBArr[gCurrent].status == BLOCKED) {
            unlock();
            schedule();
            lock();
        }
    }
    unlock();
    return 0;
}
int sem_post(sem_t *sem) {
    struct MySem *sem2 = (struct MySem*) sem->__align;
    lock();
    if(sem2->value == 0) {
        if(nextQTake != nextQAdd) {
            TCBArr[sem2->queue[nextQTake]].status = READY;
            nextQTake = (nextQTake + 1) % MAXTHREADS;
        }
    }
    sem2->value++;
    unlock();
    return 0;
}

int sem_destroy(sem_t *sem) {
    struct MySem *sem2 = (struct MySem*) sem->__align;
    lock();
    if(sem2 -> intialized == 0) return -1;
    sem2 -> intialized = 0; 
    free(sem2->queue);
    free(sem2);
    unlock();
    return 0;
}

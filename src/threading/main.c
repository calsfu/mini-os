#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "threads.h"
#include <signal.h>
#include <unistd.h>
#define THREAD_CNT 6

// //waste some time
// void *func(void *arg) {
//     printf("thread %lu: arg = %lu\n", (unsigned long)pthread_self(), (unsigned long)arg);
//     //while(1) {
//         printf("thread %lu: print\n", (unsigned long)pthread_self());
//         //pause();
//     //}
//     return arg;
// }

// int main(int argc, char **argv) {
//     pthread_t threads[THREAD_CNT];
//     unsigned long i;

//     //create THREAD_CNT threads
//     for(i = 0; i < THREAD_CNT; i++) {
//         pthread_create(&threads[i], NULL, func, (void *)(i + 100));
//         printf("thread %lu created\n", (unsigned long)(threads[i]));
//     }
//     sleep(5);
//   //  while(1) {
//         printf("thread main: print\n");
//        // sleep(3);
//   //  }

//     return 1;
// }


// waste some time
void *func(void *arg) {
    unsigned long int c = (unsigned long int)arg;
        int i;
        for (i = 0; i < c; i++) {
                if ((i % 1000) == 0) {
                        printf("tid: 0x%x counted dJust  to %d of %ld\n",         
                        (unsigned int)pthread_self(), i, c);
                   //sleep(1);
                }
        }
        printf("done counting for thread %lu\n", (unsigned long)pthread_self());
    return arg;
}
void *count(void *arg) {
        unsigned long int c = (unsigned long int)arg;
        int i;
        for (i = 0; i < c; i++) {
                if ((i % 1000) == 0) {
                        printf("tid: 0x%x Just counted to %d of %ld\n",         
                        (unsigned int)pthread_self(), i, c);
                   //sleep(1);
                }
        }
        printf("done counting for thread %lu\n", (unsigned long)pthread_self());
    return arg;
}

int main(int argc, char **argv) {
    pthread_t threads[THREAD_CNT];
    int i;
    unsigned long int cnt = 10000000;
    void *ret;
    sem_t sem;

    sem_init(&sem, 0, 2); 
    sem_wait(&sem);

    //create THRAD_CNT threads
    for(i = 0; i<THREAD_CNT - 2; i++) {
            pthread_create(&threads[i], NULL, count, (void *)((i+1)*cnt));   
    }
    unsigned long j;
    for(j = THREAD_CNT - 2; j<THREAD_CNT; j++) {
            pthread_create(&threads[j], NULL, func, (void *)((j+1)*cnt));  
    }
    // join all threads ... not important for proj2
    // int* x;
    for(i = 0; i<THREAD_CNT; i++) {
        printf("joined\n");
        pthread_join(threads[i], &ret);
    }
    
//     printf("%d\n", (int)*x);
    // sleep(5);
    // But we have to make sure that main does not return before
    // the threads are done... so count some more...
    //sleep(5);
    count((void *)(cnt*(THREAD_CNT + 1)));
    printf("exited with %lu\n", (unsigned long)ret);
    return 0;
}

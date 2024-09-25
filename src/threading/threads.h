#ifndef THREADS_H
#define THREADS_H

#include <setjmp.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>

int pthread_create(
    pthread_t *thread,
    const pthread_attr_t *attr,
    void *(*start_routine) (void *),
    void *arg
);


pthread_t pthread_self(void);

void pthread_exit(void *value_ptr);
int pthread_join(pthread_t thread, void **value_ptr);


void lock();
void unlock();

void pthread_exit_wrapper();

int sem_init(sem_t *sem, int pshared, unsigned value);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_destroy(sem_t *sem);

#endif
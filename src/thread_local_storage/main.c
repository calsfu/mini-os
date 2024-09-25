#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

// Constants
#define BUFFER_SIZE 20

// Thread-local storage functions
extern int tls_create(unsigned int size);
extern int tls_write(unsigned int offset, unsigned int length, char *buffer);
extern int tls_read(unsigned int offset, unsigned int length, char *buffer);
extern int tls_destroy();
extern int tls_clone(pthread_t tid);

// Shared buffer
char shared_buffer[BUFFER_SIZE];
pthread_t tid;
// Thread function
void* thread_function(void* arg) {
    

    // Create thread-local storage
    if (tls_create(BUFFER_SIZE) != 0) {
        fprintf(stderr, "Thread %lu: TLS creation failed\n", tid);
        pthread_exit(NULL);
    }

    // Write data to thread-local storage
   

    // Clone thread-local storage
    if (tls_clone(tid) != 0) {
        fprintf(stderr, "Thread %lu: TLS clone failed between %lu \n", pthread_self(), tid);
        // pthread_exit(NULL);
    }

    printf("Thread %lu: Cloned TLS\n", tid);
    char data_to_write[] = "H";
    if (tls_write(0, strlen(data_to_write), data_to_write) != 0) {
        fprintf(stderr, "Thread %lu: TLS write failed\n", tid);
        pthread_exit(NULL);
    }
    printf("Thread %lu: Original TLS - Wrote to TLS: %s\n", tid, data_to_write);
    // Read and print data from original TLS
    char read_buffer[1];
    if (tls_read(0, 1, read_buffer) != 0) {
        fprintf(stderr, "Thread %lu: TLS read failed\n", tid);
        pthread_exit(NULL);
    }

    printf("Thread %lu: Original TLS - Read from TLS: %s\n", tid, read_buffer);

    // Read and print data from cloned TLS
    if (tls_read(0, BUFFER_SIZE, read_buffer) != 0) {
        fprintf(stderr, "Thread %lu: TLS read failed\n", tid);
        pthread_exit(NULL);
    }

    printf("Thread %lu: Cloned TLS - Read from TLS: %s\n", tid, read_buffer);

    // Destroy thread-local storage
    if (tls_destroy() != 0) {
        fprintf(stderr, "Thread %lu: TLS destruction failed\n", tid);
        pthread_exit(NULL);
    }

    pthread_exit(NULL);
}

int main() {
    // Initialize shared buffer
    memset(shared_buffer, 0, BUFFER_SIZE);

    // Create pthreads
    pthread_t threads[3];
    int i;
    for ( i = 0; i < 3; ++i) {
        if (pthread_create(&threads[i], NULL, thread_function, NULL) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    tid = threads[2];
    // Wait for the threads to finish
    for ( i = 0; i < 3; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

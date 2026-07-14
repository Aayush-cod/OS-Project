/*
 * 1_thread_creation.c
 * --------------------
 * Demonstrates basic multi-threading in C using POSIX threads (pthreads).
 * Creates multiple threads that each perform independent, simple work
 * concurrently, then waits for all of them to complete.
 *
 * Concepts demonstrated:
 *  - pthread_create(): spawning a new thread
 *  - pthread_join(): waiting for a thread to finish (synchronizing completion)
 *  - Passing arguments to threads
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h> // for sleep()

#define NUM_THREADS 4  // minimum requirement is 3, we use 4

// This struct lets us pass a thread ID (and any future data) into each thread
typedef struct {
    int thread_id;
} thread_arg_t;

// The function each thread will run.
// pthreads requires the signature: void* function(void* arg)
void* worker_thread(void* arg) {
    thread_arg_t* data = (thread_arg_t*) arg;

    printf("Thread %d: started\n", data->thread_id);

    // Simulate some work being done (e.g., I/O, computation)
    sleep(1);

    printf("Thread %d: finished\n", data->thread_id);

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    thread_arg_t thread_args[NUM_THREADS];

    printf("Main: creating %d threads...\n\n", NUM_THREADS);

    // Create each thread
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].thread_id = i + 1;

        int result = pthread_create(
            &threads[i],          // thread handle
            NULL,                 // default thread attributes
            worker_thread,        // function each thread will run
            &thread_args[i]       // argument passed to that function
        );

        if (result != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    // Wait for all threads to finish before main() exits.
    // Without this, main() could exit before threads complete their work.
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\nMain: all threads have completed.\n");

    return 0;
}
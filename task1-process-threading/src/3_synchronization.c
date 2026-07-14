/*
 * 3_synchronization.c
 * --------------------
 * Fixes the race condition seen in 2_race_condition.c by introducing
 * a MUTEX (mutual exclusion lock) around the shared resource.
 *
 * A mutex ensures only ONE thread can execute the "critical section"
 * (the code that reads/modifies shared_counter) at any given time.
 * Other threads must wait until the lock is released.
 *
 * Result: the final value will now ALWAYS match the expected value,
 * consistently, across every run.
 */

#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 4
#define INCREMENTS_PER_THREAD 100000

long shared_counter = 0;

// The mutex that protects shared_counter.
// PTHREAD_MUTEX_INITIALIZER sets it up in an unlocked state.
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

void* increment_counter(void* arg) {
    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        // --- CRITICAL SECTION START ---
        pthread_mutex_lock(&counter_mutex);   // acquire the lock

        shared_counter++;                     // only one thread can be here at a time

        pthread_mutex_unlock(&counter_mutex); // release the lock
        // --- CRITICAL SECTION END ---
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];

    printf("Starting %d threads with MUTEX-protected access, each incrementing %d times...\n",
           NUM_THREADS, INCREMENTS_PER_THREAD);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, increment_counter, NULL);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    long expected = (long) NUM_THREADS * INCREMENTS_PER_THREAD;

    printf("\nExpected value: %ld\n", expected);
    printf("Actual value:   %ld\n", shared_counter);

    if (shared_counter == expected) {
        printf("\n>>> SUCCESS: mutex prevented the race condition. Value is correct.\n");
    } else {
        printf("\n>>> Unexpected mismatch — check mutex logic.\n");
    }

    // Clean up the mutex once we're done with it
    pthread_mutex_destroy(&counter_mutex);

    return 0;
}
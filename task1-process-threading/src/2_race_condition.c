/*
 * 2_race_condition.c
 * -------------------
 * Demonstrates a RACE CONDITION: multiple threads modifying a shared
 * resource (a global counter) with NO synchronization.
 *
 * Expected (correct) result if threads ran safely: NUM_THREADS * INCREMENTS_PER_THREAD
 * Actual result: usually LOWER than expected, and can vary between runs,
 * because threads interfere with each other while updating the shared counter.
 */

#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 4
#define INCREMENTS_PER_THREAD 100000

// Shared resource — accessed by ALL threads with no protection
long shared_counter = 0;

void* increment_counter(void* arg) {
    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        // This looks like one operation, but it's actually THREE steps:
        // 1. Read shared_counter into a register
        // 2. Add 1 to it
        // 3. Write it back to shared_counter
        // If two threads interleave these steps, updates get lost.
        shared_counter++;
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];

    printf("Starting %d threads, each incrementing shared_counter %d times...\n",
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

    if (shared_counter != expected) {
        printf("\n>>> RACE CONDITION DETECTED: lost updates due to unsynchronized access!\n");
    } else {
        printf("\n>>> No race condition observed this run (can still happen — try re-running).\n");
    }

    return 0;
}
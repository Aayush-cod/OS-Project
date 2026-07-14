/*
 * 5_deadlock_prevention.c
 * ------------------------
 * Prevents the deadlock seen in 4_deadlock.c using LOCK ORDERING.
 *
 * Rule: ALL threads must acquire mutex_1 BEFORE mutex_2, always,
 * with no exceptions. This eliminates the "circular wait" condition
 * (one of the four necessary conditions for deadlock), because it's
 * now impossible for one thread to hold mutex_2 while waiting on
 * mutex_1, and another to hold mutex_1 while waiting on mutex_2.
 *
 * Thread B previously locked mutex_2 first — we simply flip its
 * order to match Thread A. Both now agree on a global lock order.
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;

void* thread_A(void* arg) {
    printf("Thread A: trying to lock mutex_1...\n");
    pthread_mutex_lock(&mutex_1);
    printf("Thread A: locked mutex_1\n");

    sleep(1);

    printf("Thread A: trying to lock mutex_2...\n");
    pthread_mutex_lock(&mutex_2);
    printf("Thread A: locked mutex_2\n");

    pthread_mutex_unlock(&mutex_2);
    pthread_mutex_unlock(&mutex_1);
    printf("Thread A: done, released both locks\n");
    return NULL;
}

void* thread_B(void* arg) {
    // FIX: Thread B now locks mutex_1 FIRST too, same order as Thread A.
    // (Previously this locked mutex_2 first — that mismatch caused the deadlock.)
    printf("Thread B: trying to lock mutex_1...\n");
    pthread_mutex_lock(&mutex_1);
    printf("Thread B: locked mutex_1\n");

    sleep(1);

    printf("Thread B: trying to lock mutex_2...\n");
    pthread_mutex_lock(&mutex_2);
    printf("Thread B: locked mutex_2\n");

    pthread_mutex_unlock(&mutex_2);
    pthread_mutex_unlock(&mutex_1);
    printf("Thread B: done, released both locks\n");
    return NULL;
}

int main() {
    pthread_t tA, tB;

    printf("Main: starting Thread A and Thread B (deadlock PREVENTED via lock ordering)...\n\n");

    pthread_create(&tA, NULL, thread_A, NULL);
    pthread_create(&tB, NULL, thread_B, NULL);

    pthread_join(tA, NULL);
    pthread_join(tB, NULL);

    printf("\nMain: both threads finished successfully. No deadlock occurred.\n");

    return 0;
}
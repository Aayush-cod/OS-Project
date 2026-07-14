/*
 * 4_deadlock.c
 * ------------
 * Intentionally demonstrates a DEADLOCK using two mutexes locked
 * in opposite order by two different threads.
 *
 * Thread A: locks mutex_1, then tries to lock mutex_2
 * Thread B: locks mutex_2, then tries to lock mutex_1
 *
 * If the timing lines up (which sleep() here forces), each thread
 * ends up holding one lock while waiting forever for the other.
 * This satisfies the "circular wait" condition required for deadlock.
 *
 * WARNING: This program will HANG. You will need to press Ctrl+C
 * to terminate it manually — that hang IS the demonstration.
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h> // for sleep()

pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;

void* thread_A(void* arg) {
    printf("Thread A: trying to lock mutex_1...\n");
    pthread_mutex_lock(&mutex_1);
    printf("Thread A: locked mutex_1\n");

    // Force a delay so Thread B has time to lock mutex_2 first,
    // guaranteeing the deadlock scenario happens (not just possibly happens)
    sleep(1);

    printf("Thread A: trying to lock mutex_2...\n");
    pthread_mutex_lock(&mutex_2);  // <-- will block forever here
    printf("Thread A: locked mutex_2\n");

    pthread_mutex_unlock(&mutex_2);
    pthread_mutex_unlock(&mutex_1);
    return NULL;
}

void* thread_B(void* arg) {
    printf("Thread B: trying to lock mutex_2...\n");
    pthread_mutex_lock(&mutex_2);
    printf("Thread B: locked mutex_2\n");

    sleep(1);

    printf("Thread B: trying to lock mutex_1...\n");
    pthread_mutex_lock(&mutex_1);  // <-- will block forever here
    printf("Thread B: locked mutex_1\n");

    pthread_mutex_unlock(&mutex_1);
    pthread_mutex_unlock(&mutex_2);
    return NULL;
}

int main() {
    pthread_t tA, tB;

    printf("Main: starting Thread A and Thread B (deadlock expected)...\n\n");

    pthread_create(&tA, NULL, thread_A, NULL);
    pthread_create(&tB, NULL, thread_B, NULL);

    pthread_join(tA, NULL);
    pthread_join(tB, NULL);

    // This line will NEVER print — the program deadlocks before reaching here
    printf("\nMain: both threads finished (you should never see this line).\n");

    return 0;
}
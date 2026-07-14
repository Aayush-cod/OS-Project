/*
 * 6_scheduler.c
 * -------------
 * Simulates a Round-Robin CPU scheduling algorithm.
 *
 * Each process has a total "burst time" (how much CPU time it needs
 * to finish). The scheduler gives each process a fixed TIME_QUANTUM
 * per turn. If a process doesn't finish within its quantum, it goes
 * to the back of the queue and waits for its next turn.
 *
 * This demonstrates fair CPU time-sharing among multiple processes,
 * a core OS scheduling concept.
 */

#include <stdio.h>

#define MAX_PROCESSES 5
#define TIME_QUANTUM 3   // each process gets 3 units of CPU time per turn

typedef struct {
    int pid;                 // process ID
    int burst_time;          // total CPU time originally needed
    int remaining_time;      // CPU time still needed
    int completion_time;     // time at which process finished
} Process;

void round_robin(Process processes[], int n) {
    int time_elapsed = 0;
    int completed = 0;

    // Keep looping through the queue until every process is done
    while (completed < n) {
        int all_done_this_pass = 1; // tracks if we made any progress this pass

        for (int i = 0; i < n; i++) {
            if (processes[i].remaining_time > 0) {
                all_done_this_pass = 0;

                int time_slice = (processes[i].remaining_time < TIME_QUANTUM)
                                    ? processes[i].remaining_time
                                    : TIME_QUANTUM;

                printf("Time %2d: Process P%d runs for %d unit(s) "
                       "(remaining before: %d)\n",
                       time_elapsed, processes[i].pid, time_slice,
                       processes[i].remaining_time);

                processes[i].remaining_time -= time_slice;
                time_elapsed += time_slice;

                if (processes[i].remaining_time == 0) {
                    processes[i].completion_time = time_elapsed;
                    completed++;
                    printf("         -> Process P%d COMPLETED at time %d\n",
                           processes[i].pid, time_elapsed);
                }
            }
        }

        // Safety net: prevents infinite loop if something's misconfigured
        if (all_done_this_pass) break;
    }

    // Print summary table
    printf("\n--- Round Robin Scheduling Summary (Quantum = %d) ---\n", TIME_QUANTUM);
    printf("PID\tBurst Time\tCompletion Time\tTurnaround Time\n");

    float total_turnaround = 0;
    for (int i = 0; i < n; i++) {
        int turnaround = processes[i].completion_time; // arrival time assumed 0
        total_turnaround += turnaround;
        printf("P%d\t%d\t\t%d\t\t\t%d\n",
               processes[i].pid, processes[i].burst_time,
               processes[i].completion_time, turnaround);
    }

    printf("\nAverage Turnaround Time: %.2f units\n", total_turnaround / n);
}

int main() {
    // Define our sample processes with different burst times
    Process processes[MAX_PROCESSES] = {
        {1, 10, 10, 0},
        {2, 5, 5, 0},
        {3, 8, 8, 0},
        {4, 4, 4, 0},
        {5, 6, 6, 0}
    };

    int n = MAX_PROCESSES;

    printf("Simulating Round Robin Scheduling for %d processes...\n\n", n);

    round_robin(processes, n);

    return 0;
}
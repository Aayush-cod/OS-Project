#include <stdio.h>

#define NUM_FRAMES  3
#define EMPTY       -1

typedef struct {
    int page_number;
    int last_used;   // logical timestamp of this frame's most recent access
} Frame;

int logical_clock = 0; // increments on every single page access

// Checks if a page is already loaded; returns the frame index, or -1 if not found
int find_page(Frame memory[], int page) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (memory[i].page_number == page) return i;
    }
    return -1;
}

// Checks if there's a free (empty) frame available; returns its index, or -1 if full
int find_free_frame(Frame memory[]) {
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (memory[i].page_number == EMPTY) return i;
    }
    return -1;
}

// Scans all frames and returns the index of the one with the SMALLEST
// last_used value - i.e. the frame that was used least recently
int find_lru_victim(Frame memory[]) {
    int victim = 0;
    for (int i = 1; i < NUM_FRAMES; i++) {
        if (memory[i].last_used < memory[victim].last_used) {
            victim = i;
        }
    }
    return victim;
}

// Handles one page access using LRU replacement.
// Returns 1 if it was a page fault, 0 if it was a hit.
int access_page_lru(Frame memory[], int page) {
    logical_clock++; // every access moves the logical clock forward

    int frame = find_page(memory, page);

    if (frame != -1) {
        // HIT: page already loaded - just refresh its "last used" timestamp
        memory[frame].last_used = logical_clock;
        printf("Access page %d -> HIT  (frame %d, last_used updated to %d)\n",
               page, frame, logical_clock);
        return 0;
    }

    // Page fault: need to load it somewhere
    int free_frame = find_free_frame(memory);

    if (free_frame != -1) {
        memory[free_frame].page_number = page;
        memory[free_frame].last_used = logical_clock;
        printf("Access page %d -> FAULT (loaded into free frame %d)\n", page, free_frame);
    } else {
        // Memory is full: evict the LEAST RECENTLY USED page
        int victim_frame = find_lru_victim(memory);
        int evicted_page = memory[victim_frame].page_number;

        memory[victim_frame].page_number = page;
        memory[victim_frame].last_used = logical_clock;

        printf("Access page %d -> FAULT (evicted page %d from frame %d, LRU)\n",
               page, evicted_page, victim_frame);
    }

    return 1;
}

void print_memory_state(Frame memory[]) {
    printf("   Memory state: [ ");
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (memory[i].page_number == EMPTY)
            printf("empty ");
        else
            printf("P%d(t%d) ", memory[i].page_number, memory[i].last_used);
    }
    printf("]\n\n");
}

int main() {
    Frame memory[NUM_FRAMES];
    for (int i = 0; i < NUM_FRAMES; i++) {
        memory[i].page_number = EMPTY;
        memory[i].last_used = 0;
    }

    // IDENTICAL reference string to 2_fifo_replacement.c, so results are
    // directly comparable between the two algorithms.
    int reference_string[] = {1, 2, 3, 1, 2, 4, 5, 1};
    int num_references = sizeof(reference_string) / sizeof(reference_string[0]);

    int faults = 0, hits = 0;

    printf("Simulating LRU Page Replacement with NUM_FRAMES=%d\n\n", NUM_FRAMES);

    for (int i = 0; i < num_references; i++) {
        int page = reference_string[i];
        int was_fault = access_page_lru(memory, page);

        if (was_fault) faults++; else hits++;

        print_memory_state(memory);
    }

    printf("--- LRU Summary ---\n");
    printf("Total references: %d\n", num_references);
    printf("Hits:   %d\n", hits);
    printf("Faults: %d\n", faults);
    printf("Hit ratio:  %.2f%%\n", (hits / (float) num_references) * 100);
    printf("Fault ratio: %.2f%%\n", (faults / (float) num_references) * 100);

    return 0;
}
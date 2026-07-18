#include <stdio.h>

#define NUM_FRAMES  3
#define EMPTY       -1

typedef struct {
    int page_number;
} Frame;

// FIFO queue: fifo_queue[0] is the frame that was loaded LEAST recently
// (i.e. next in line to be evicted). New loads are appended at the end.
int fifo_queue[NUM_FRAMES];
int queue_size = 0;

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

// Adds a frame index to the back of the FIFO queue
void fifo_enqueue(int frame_index) {
    fifo_queue[queue_size] = frame_index;
    queue_size++;
}

// Removes and returns the frame index at the front of the queue
// (the frame that was loaded longest ago), shifting the rest forward
int fifo_dequeue() {
    int evicted_frame = fifo_queue[0];
    for (int i = 0; i < queue_size - 1; i++) {
        fifo_queue[i] = fifo_queue[i + 1];
    }
    queue_size--;
    return evicted_frame;
}

// Handles one page access using FIFO replacement.
// Returns 1 if it was a page fault, 0 if it was a hit.
int access_page_fifo(Frame memory[], int page) {
    int frame = find_page(memory, page);

    if (frame != -1) {
        printf("Access page %d -> HIT  (frame %d)\n", page, frame);
        return 0;
    }

    // Page fault: need to load it somewhere
    int free_frame = find_free_frame(memory);

    if (free_frame != -1) {
        // There's still a free frame, no eviction needed yet
        memory[free_frame].page_number = page;
        fifo_enqueue(free_frame);
        printf("Access page %d -> FAULT (loaded into free frame %d)\n", page, free_frame);
    } else {
        // Memory is full: evict the oldest-loaded page (front of FIFO queue)
        int victim_frame = fifo_dequeue();
        int evicted_page = memory[victim_frame].page_number;

        memory[victim_frame].page_number = page;
        fifo_enqueue(victim_frame); // the new page is now the "newest" in this frame

        printf("Access page %d -> FAULT (evicted page %d from frame %d, FIFO)\n",
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
            printf("P%d ", memory[i].page_number);
    }
    printf("]\n\n");
}

int main() {
    Frame memory[NUM_FRAMES];
    for (int i = 0; i < NUM_FRAMES; i++) {
        memory[i].page_number = EMPTY;
    }

    // Same reference string as Stage A, but extended further so we can
    // clearly observe FIFO evictions happening (not just the "memory full"
    // message we saw before).
    int reference_string[] = {1, 2, 3, 1, 2, 4, 5, 1};
    int num_references = sizeof(reference_string) / sizeof(reference_string[0]);

    int faults = 0, hits = 0;

    printf("Simulating FIFO Page Replacement with NUM_FRAMES=%d\n\n", NUM_FRAMES);

    for (int i = 0; i < num_references; i++) {
        int page = reference_string[i];
        int was_fault = access_page_fifo(memory, page);

        if (was_fault) faults++; else hits++;

        print_memory_state(memory);
    }

    printf("--- FIFO Summary ---\n");
    printf("Total references: %d\n", num_references);
    printf("Hits:   %d\n", hits);
    printf("Faults: %d\n", faults);
    printf("Hit ratio:  %.2f%%\n", (hits / (float) num_references) * 100);
    printf("Fault ratio: %.2f%%\n", (faults / (float) num_references) * 100);

    return 0;
}
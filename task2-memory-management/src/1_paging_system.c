
#include <stdio.h>

#define PAGE_SIZE   4096   // configurable page size in bytes (4 KB, a common real-world default)
#define NUM_FRAMES  3      // number of physical memory frames available (kept small to force faults easily)
#define EMPTY       -1     // sentinel value meaning "this frame holds no page"

// Represents one physical memory frame
typedef struct {
    int page_number;   // which virtual page is currently loaded here (-1 if empty)
} Frame;

// Simulates loading a page into memory and reports whether it was a
// hit (already present) or a fault (had to be loaded into a free frame).
// Returns 1 if it was a page fault, 0 if it was a hit.
int access_page(Frame memory[], int page) {
    // Step 1: check if the page is already loaded (a HIT)
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (memory[i].page_number == page) {
            printf("Access page %d -> HIT  (already in frame %d)\n", page, i);
            return 0;
        }
    }

    // Step 2: not found -> this is a PAGE FAULT. Find a free frame.
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (memory[i].page_number == EMPTY) {
            memory[i].page_number = page;
            printf("Access page %d -> FAULT (loaded into free frame %d)\n", page, i);
            return 1;
        }
    }

    // Step 3: no free frame available. In this basic stage we simply
    // report that memory is full — replacement algorithms (later stages)
    // will decide which page to evict to make room.
    printf("Access page %d -> FAULT (memory full, no free frame - "
           "replacement needed, see later stages)\n", page);
    return 1;
}

// Prints the current contents of all frames, useful for tracing behaviour
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
    // Physical memory: NUM_FRAMES frames, all start empty
    Frame memory[NUM_FRAMES];
    for (int i = 0; i < NUM_FRAMES; i++) {
        memory[i].page_number = EMPTY;
    }

    // A page reference string: the sequence of virtual pages the
    // "process" accesses over time. This particular sequence includes
    // repeats (to produce hits) and enough distinct pages to fill and
    // exceed the available frames (to produce faults).
    int reference_string[] = {1, 2, 3, 1, 2, 4};
    int num_references = sizeof(reference_string) / sizeof(reference_string[0]);

    int faults = 0, hits = 0;

    printf("Simulating paging with PAGE_SIZE=%d bytes, NUM_FRAMES=%d\n\n",
           PAGE_SIZE, NUM_FRAMES);

    for (int i = 0; i < num_references; i++) {
        int page = reference_string[i];
        int was_fault = access_page(memory, page);

        if (was_fault) faults++; else hits++;

        print_memory_state(memory);
    }

    printf("--- Summary ---\n");
    printf("Total references: %d\n", num_references);
    printf("Hits:   %d\n", hits);
    printf("Faults: %d\n", faults);
    printf("Hit ratio:  %.2f%%\n", (hits / (float) num_references) * 100);
    printf("Fault ratio: %.2f%%\n", (faults / (float) num_references) * 100);

    return 0;
}
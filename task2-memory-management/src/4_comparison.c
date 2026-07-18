#include <stdio.h>

#define MAX_FRAMES 10
#define EMPTY      -1

typedef struct {
    int page_number;
} FifoFrame;

typedef struct {
    int page_number;
    int last_used;
} LruFrame;

// ---------- FIFO implementation (reusable for any reference string/frame count) ----------
int run_fifo(int reference[], int n, int num_frames, int verbose) {
    FifoFrame memory[MAX_FRAMES];
    int fifo_queue[MAX_FRAMES], queue_size = 0;
    for (int i = 0; i < num_frames; i++) memory[i].page_number = EMPTY;

    int faults = 0;

    for (int i = 0; i < n; i++) {
        int page = reference[i];
        int found = -1;
        for (int f = 0; f < num_frames; f++)
            if (memory[f].page_number == page) { found = f; break; }

        if (found != -1) {
            if (verbose) printf("  [FIFO] page %d -> HIT (frame %d)\n", page, found);
            continue;
        }

        faults++;
        int free_frame = -1;
        for (int f = 0; f < num_frames; f++)
            if (memory[f].page_number == EMPTY) { free_frame = f; break; }

        if (free_frame != -1) {
            memory[free_frame].page_number = page;
            fifo_queue[queue_size++] = free_frame;
            if (verbose) printf("  [FIFO] page %d -> FAULT (free frame %d)\n", page, free_frame);
        } else {
            int victim = fifo_queue[0];
            for (int q = 0; q < queue_size - 1; q++) fifo_queue[q] = fifo_queue[q + 1];
            queue_size--;

            int evicted = memory[victim].page_number;
            memory[victim].page_number = page;
            fifo_queue[queue_size++] = victim;
            if (verbose) printf("  [FIFO] page %d -> FAULT (evicted %d, frame %d)\n", page, evicted, victim);
        }
    }
    return faults;
}

// ---------- LRU implementation (reusable for any reference string/frame count) ----------
int run_lru(int reference[], int n, int num_frames, int verbose) {
    LruFrame memory[MAX_FRAMES];
    for (int i = 0; i < num_frames; i++) { memory[i].page_number = EMPTY; memory[i].last_used = 0; }

    int faults = 0, clock = 0;

    for (int i = 0; i < n; i++) {
        int page = reference[i];
        clock++;
        int found = -1;
        for (int f = 0; f < num_frames; f++)
            if (memory[f].page_number == page) { found = f; break; }

        if (found != -1) {
            memory[found].last_used = clock;
            if (verbose) printf("  [LRU]  page %d -> HIT (frame %d)\n", page, found);
            continue;
        }

        faults++;
        int free_frame = -1;
        for (int f = 0; f < num_frames; f++)
            if (memory[f].page_number == EMPTY) { free_frame = f; break; }

        if (free_frame != -1) {
            memory[free_frame].page_number = page;
            memory[free_frame].last_used = clock;
            if (verbose) printf("  [LRU]  page %d -> FAULT (free frame %d)\n", page, free_frame);
        } else {
            int victim = 0;
            for (int f = 1; f < num_frames; f++)
                if (memory[f].last_used < memory[victim].last_used) victim = f;

            int evicted = memory[victim].page_number;
            memory[victim].page_number = page;
            memory[victim].last_used = clock;
            if (verbose) printf("  [LRU]  page %d -> FAULT (evicted %d, frame %d)\n", page, evicted, victim);
        }
    }
    return faults;
}

void print_reference_string(int reference[], int n) {
    printf("[ ");
    for (int i = 0; i < n; i++) printf("%d ", reference[i]);
    printf("]\n");
}

void run_test_case(const char* name, int reference[], int n, int num_frames) {
    printf("=== %s ===\n", name);
    printf("Reference string: ");
    print_reference_string(reference, n);
    printf("Frames: %d | Total references: %d\n\n", num_frames, n);

    printf("--- FIFO trace ---\n");
    int fifo_faults = run_fifo(reference, n, num_frames, 1);

    printf("\n--- LRU trace ---\n");
    int lru_faults = run_lru(reference, n, num_frames, 1);

    int fifo_hits = n - fifo_faults;
    int lru_hits  = n - lru_faults;

    printf("\n--- Comparison ---\n");
    printf("%-12s %10s %10s %12s\n", "Algorithm", "Faults", "Hits", "Hit Ratio");
    printf("%-12s %10d %10d %11.2f%%\n", "FIFO", fifo_faults, fifo_hits, (fifo_hits / (float) n) * 100);
    printf("%-12s %10d %10d %11.2f%%\n", "LRU",  lru_faults,  lru_hits,  (lru_hits  / (float) n) * 100);

    if (fifo_faults < lru_faults)
        printf("\nResult: FIFO performed better on this reference string.\n");
    else if (lru_faults < fifo_faults)
        printf("\nResult: LRU performed better on this reference string.\n");
    else
        printf("\nResult: FIFO and LRU performed identically on this reference string.\n");

    printf("\n=================================================================\n\n");
}

int main() {
    // Test Case 1: original string used in Stages B and C (baseline comparison)
    int ref1[] = {1, 2, 3, 1, 2, 4, 5, 1};
    run_test_case("Test Case 1: Baseline (from Stages B & C)", ref1, 8, 3);

    // Test Case 2: a longer string with strong "locality of reference"
    // (repeated recent pages) - the kind of pattern LRU is designed to
    // exploit, expected to favour LRU.
    int ref2[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    run_test_case("Test Case 2: Locality-heavy access pattern", ref2, 12, 3);

    // Test Case 3: sequential access with no repeats until the very end -
    // a worst-case-like pattern for both algorithms, useful to show that
    // with zero reuse, replacement strategy barely matters.
    int ref3[] = {1, 2, 3, 4, 5, 6, 7, 1};
    run_test_case("Test Case 3: Mostly sequential, low reuse", ref3, 8, 3);

    return 0;
}
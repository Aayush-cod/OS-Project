#include <stdio.h>
#include <string.h>

#define MAX_FILES 10
#define MAX_NAME  64

// Simple in-memory ownership registry (a real system would persist this;
// kept in-memory here to keep the demo self-contained and readable)
typedef struct {
    char filename[MAX_NAME];
    char owner[MAX_NAME];
    int in_use;
} FileRecord;

FileRecord registry[MAX_FILES];
int file_count = 0;

// Finds a file's registry entry by name; returns index, or -1 if not found
int find_file(const char* filename) {
    for (int i = 0; i < file_count; i++) {
        if (registry[i].in_use && strcmp(registry[i].filename, filename) == 0) {
            return i;
        }
    }
    return -1;
}

// Creates a new file on disk and records the session user as its owner
void create_file(const char* current_user, const char* filename, const char* content) {
    if (find_file(filename) != -1) {
        printf("[CREATE] Failed: '%s' already exists.\n", filename);
        return;
    }

    FILE* f = fopen(filename, "w");
    if (f == NULL) {
        printf("[CREATE] Failed: could not create '%s' on disk.\n", filename);
        return;
    }
    fprintf(f, "%s", content);
    fclose(f);

    strcpy(registry[file_count].filename, filename);
    strcpy(registry[file_count].owner, current_user);
    registry[file_count].in_use = 1;
    file_count++;

    printf("[CREATE] '%s' created by '%s'.\n", filename, current_user);
}

// Reads and prints a file's contents (any logged-in user may read, in
// this stage - read restriction by permission is added in Stage C)
void read_file(const char* current_user, const char* filename) {
    int idx = find_file(filename);
    if (idx == -1) {
        printf("[READ]   Failed: '%s' does not exist.\n", filename);
        return;
    }

    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        printf("[READ]   Failed: could not open '%s'.\n", filename);
        return;
    }

    char buffer[256];
    printf("[READ]   '%s' by '%s' -> contents: \"", filename, current_user);
    while (fgets(buffer, sizeof(buffer), f)) {
        printf("%s", buffer);
    }
    printf("\"\n");
    fclose(f);
}

// Overwrites a file's contents - only the owner is permitted
void write_file(const char* current_user, const char* filename, const char* new_content) {
    int idx = find_file(filename);
    if (idx == -1) {
        printf("[WRITE]  Failed: '%s' does not exist.\n", filename);
        return;
    }

    if (strcmp(registry[idx].owner, current_user) != 0) {
        printf("[WRITE]  DENIED: '%s' is not the owner of '%s' (owner: %s).\n",
               current_user, filename, registry[idx].owner);
        return;
    }

    FILE* f = fopen(filename, "w");
    fprintf(f, "%s", new_content);
    fclose(f);

    printf("[WRITE]  '%s' updated by '%s'.\n", filename, current_user);
}

// Deletes a file - only the owner is permitted
void delete_file(const char* current_user, const char* filename) {
    int idx = find_file(filename);
    if (idx == -1) {
        printf("[DELETE] Failed: '%s' does not exist.\n", filename);
        return;
    }

    if (strcmp(registry[idx].owner, current_user) != 0) {
        printf("[DELETE] DENIED: '%s' is not the owner of '%s' (owner: %s).\n",
               current_user, filename, registry[idx].owner);
        return;
    }

    remove(filename);
    registry[idx].in_use = 0;
    printf("[DELETE] '%s' deleted by '%s'.\n", filename, current_user);
}

int main() {
    // Simulate two already-authenticated sessions (in the integrated
    // system, Stage F, this would come from a real login via Stage A)
    const char* alice = "alice";
    const char* bob = "bob";

    printf("--- File operations as 'alice' ---\n");
    create_file(alice, "notes.txt", "Alice's private notes.\n");
    read_file(alice, "notes.txt");
    write_file(alice, "notes.txt", "Alice's UPDATED notes.\n");

    printf("\n--- 'bob' tries to access alice's file ---\n");
    read_file(bob, "notes.txt");                 // allowed in this stage
    write_file(bob, "notes.txt", "hacked!");      // should be denied
    delete_file(bob, "notes.txt");                // should be denied

    printf("\n--- 'alice' deletes her own file ---\n");
    delete_file(alice, "notes.txt");
    read_file(alice, "notes.txt");                // should now fail, file gone

    return 0;
}
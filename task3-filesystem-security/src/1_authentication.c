#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define USERS_FILE "users.txt"
#define MAX_LINE 256

// djb2: a simple, fast string hash function (not cryptographically
// secure - see note above). Returns an unsigned long hash of the input.
unsigned long hash_password(const char* password) {
    unsigned long hash = 5381;
    int c;
    while ((c = *password++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// Registers a new user by appending "username:hash" to the users file.
// Returns 1 on success, 0 if the username already exists.
int register_user(const char* username, const char* password) {
    // Check if username already exists
    FILE* check = fopen(USERS_FILE, "r");
    if (check != NULL) {
        char line[MAX_LINE];
        while (fgets(line, sizeof(line), check)) {
            char existing_user[MAX_LINE];
            sscanf(line, "%[^:]", existing_user);
            if (strcmp(existing_user, username) == 0) {
                fclose(check);
                printf("Registration failed: username '%s' already exists.\n", username);
                return 0;
            }
        }
        fclose(check);
    }

    FILE* file = fopen(USERS_FILE, "a"); // append mode
    if (file == NULL) {
        printf("Error: could not open users file.\n");
        return 0;
    }

    unsigned long hashed = hash_password(password);
    fprintf(file, "%s:%lu\n", username, hashed);
    fclose(file);

    printf("User '%s' registered successfully (password stored as hash: %lu)\n",
           username, hashed);
    return 1;
}

// Verifies a login attempt against the stored hash.
// Returns 1 if credentials are valid, 0 otherwise.
int login(const char* username, const char* password) {
    FILE* file = fopen(USERS_FILE, "r");
    if (file == NULL) {
        printf("Login failed: no users registered yet.\n");
        return 0;
    }

    unsigned long attempted_hash = hash_password(password);

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        char stored_user[MAX_LINE];
        unsigned long stored_hash;
        sscanf(line, "%[^:]:%lu", stored_user, &stored_hash);

        if (strcmp(stored_user, username) == 0) {
            fclose(file);
            if (stored_hash == attempted_hash) {
                printf("Login SUCCESS for user '%s'.\n", username);
                return 1;
            } else {
                printf("Login FAILED for user '%s': incorrect password.\n", username);
                return 0;
            }
        }
    }

    fclose(file);
    printf("Login FAILED: user '%s' not found.\n", username);
    return 0;
}

int main() {
    // Start with a clean users file each run, so this demo is repeatable
    remove(USERS_FILE);

    printf("--- Registering users ---\n");
    register_user("alice", "correcthorse123");
    register_user("bob", "hunter2");
    register_user("alice", "duplicateattempt"); // should fail - username taken

    printf("\n--- Login attempts ---\n");
    login("alice", "correcthorse123");  // correct -> success
    login("alice", "wrongpassword");    // wrong password -> fail
    login("charlie", "whatever");       // user doesn't exist -> fail
    login("bob", "hunter2");            // correct -> success

    return 0;
}
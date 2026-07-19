#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX_USERS 10
#define MAX_FILES 10
#define MAX_NAME  64
#define USERS_FILE "sys_users.txt"
#define LOG_FILE   "sys_audit.log"

/* ---------------- Data structures ---------------- */

typedef struct {
    char username[MAX_NAME];
    char group[MAX_NAME];
    unsigned long password_hash;
    int in_use;
} User;

typedef struct {
    int read, write, execute;
} PermissionSet;

typedef struct {
    char filename[MAX_NAME];
    char owner[MAX_NAME];
    char group[MAX_NAME];
    PermissionSet owner_perms, group_perms, others_perms;
    int is_encrypted;
    int in_use;
} SecureFile;

User users[MAX_USERS];
int user_count = 0;

SecureFile files[MAX_FILES];
int file_count = 0;

char current_user[MAX_NAME] = "";   // "" means nobody logged in
char current_group[MAX_NAME] = "";

/* ---------------- Audit logging (from Stage E) ---------------- */

void log_event(const char* user, const char* action, const char* result, const char* detail) {
    FILE* log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    fprintf(log, "[%s] user=%-8s action=%-10s result=%-8s detail=%s\n",
            timestamp, user, action, result, detail);
    fclose(log);
}

/* ---------------- Authentication (from Stage A) ---------------- */

unsigned long hash_password(const char* password) {
    unsigned long hash = 5381;
    int c;
    while ((c = *password++)) hash = ((hash << 5) + hash) + c;
    return hash;
}

void register_user(const char* username, const char* password, const char* group) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            printf("[REGISTER] Failed: '%s' already exists.\n", username);
            log_event(username, "REGISTER", "FAILED", "username already exists");
            return;
        }
    }
    strcpy(users[user_count].username, username);
    strcpy(users[user_count].group, group);
    users[user_count].password_hash = hash_password(password);
    users[user_count].in_use = 1;
    user_count++;

    printf("[REGISTER] '%s' registered (group: %s).\n", username, group);
    log_event(username, "REGISTER", "SUCCESS", "new user created");
}

int login(const char* username, const char* password) {
    unsigned long attempt = hash_password(password);
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if (users[i].password_hash == attempt) {
                strcpy(current_user, username);
                strcpy(current_group, users[i].group);
                printf("[LOGIN] '%s' logged in successfully.\n", username);
                log_event(username, "LOGIN", "SUCCESS", "correct credentials");
                return 1;
            } else {
                printf("[LOGIN] Failed: incorrect password for '%s'.\n", username);
                log_event(username, "LOGIN", "FAILED", "incorrect password");
                return 0;
            }
        }
    }
    printf("[LOGIN] Failed: user '%s' not found.\n", username);
    log_event(username, "LOGIN", "FAILED", "user not found");
    return 0;
}

void logout() {
    printf("[LOGOUT] '%s' logged out.\n", current_user);
    log_event(current_user, "LOGOUT", "SUCCESS", "session ended");
    current_user[0] = '\0';
    current_group[0] = '\0';
}

/* ---------------- Permissions (from Stage C) ---------------- */

int find_file(const char* filename) {
    for (int i = 0; i < file_count; i++)
        if (files[i].in_use && strcmp(files[i].filename, filename) == 0) return i;
    return -1;
}

PermissionSet get_applicable_perms(SecureFile* f) {
    if (strcmp(current_user, f->owner) == 0) return f->owner_perms;
    if (strcmp(current_group, f->group) == 0) return f->group_perms;
    return f->others_perms;
}

int has_permission(SecureFile* f, char action) {
    PermissionSet p = get_applicable_perms(f);
    if (action == 'r') return p.read;
    if (action == 'w') return p.write;
    return p.execute;
}

/* ---------------- File operations (from Stage B) + Encryption (Stage D) ---------------- */

void xor_cipher(unsigned char* data, int len, const char* key) {
    int klen = strlen(key);
    for (int i = 0; i < len; i++) data[i] = data[i] ^ key[i % klen];
}

// Requires an active login session before any file operation proceeds
int require_login(const char* action) {
    if (strlen(current_user) == 0) {
        printf("[%s] DENIED: no user is logged in.\n", action);
        log_event("(none)", action, "DENIED", "no active session");
        return 0;
    }
    return 1;
}

void create_secure_file(const char* filename, const char* content,
                         PermissionSet owner_p, PermissionSet group_p, PermissionSet others_p) {
    if (!require_login("CREATE")) return;
    if (find_file(filename) != -1) {
        printf("[CREATE] Failed: '%s' already exists.\n", filename);
        log_event(current_user, "CREATE", "FAILED", "file already exists");
        return;
    }

    FILE* f = fopen(filename, "w");
    fprintf(f, "%s", content);
    fclose(f);

    SecureFile* sf = &files[file_count];
    strcpy(sf->filename, filename);
    strcpy(sf->owner, current_user);
    strcpy(sf->group, current_group);
    sf->owner_perms = owner_p;
    sf->group_perms = group_p;
    sf->others_perms = others_p;
    sf->is_encrypted = 0;
    sf->in_use = 1;
    file_count++;

    printf("[CREATE] '%s' created by '%s'.\n", filename, current_user);
    log_event(current_user, "CREATE", "SUCCESS", filename);
}

void read_secure_file(const char* filename) {
    if (!require_login("READ")) return;
    int idx = find_file(filename);
    if (idx == -1) {
        printf("[READ] Failed: '%s' not found.\n", filename);
        log_event(current_user, "READ", "FAILED", "file not found");
        return;
    }
    if (!has_permission(&files[idx], 'r')) {
        printf("[READ] DENIED for '%s' on '%s'.\n", current_user, filename);
        log_event(current_user, "READ", "DENIED", filename);
        return;
    }

    FILE* f = fopen(filename, "r");
    char buf[256];
    printf("[READ] '%s' by '%s' -> \"", filename, current_user);
    while (fgets(buf, sizeof(buf), f)) printf("%s", buf);
    printf("\"\n");
    fclose(f);
    log_event(current_user, "READ", "SUCCESS", filename);
}

void write_secure_file(const char* filename, const char* new_content) {
    if (!require_login("WRITE")) return;
    int idx = find_file(filename);
    if (idx == -1) {
        printf("[WRITE] Failed: '%s' not found.\n", filename);
        log_event(current_user, "WRITE", "FAILED", "file not found");
        return;
    }
    if (!has_permission(&files[idx], 'w')) {
        printf("[WRITE] DENIED for '%s' on '%s'.\n", current_user, filename);
        log_event(current_user, "WRITE", "DENIED", filename);
        return;
    }

    FILE* f = fopen(filename, "w");
    fprintf(f, "%s", new_content);
    fclose(f);
    printf("[WRITE] '%s' updated by '%s'.\n", filename, current_user);
    log_event(current_user, "WRITE", "SUCCESS", filename);
}

void delete_secure_file(const char* filename) {
    if (!require_login("DELETE")) return;
    int idx = find_file(filename);
    if (idx == -1) {
        printf("[DELETE] Failed: '%s' not found.\n", filename);
        log_event(current_user, "DELETE", "FAILED", "file not found");
        return;
    }
    if (!has_permission(&files[idx], 'w')) { // deletion requires write access
        printf("[DELETE] DENIED for '%s' on '%s'.\n", current_user, filename);
        log_event(current_user, "DELETE", "DENIED", filename);
        return;
    }

    remove(filename);
    files[idx].in_use = 0;
    printf("[DELETE] '%s' deleted by '%s'.\n", filename, current_user);
    log_event(current_user, "DELETE", "SUCCESS", filename);
}

void encrypt_secure_file(const char* filename, const char* key) {
    if (!require_login("ENCRYPT")) return;
    int idx = find_file(filename);
    if (idx == -1 || !has_permission(&files[idx], 'w')) {
        printf("[ENCRYPT] DENIED or file not found: '%s'.\n", filename);
        log_event(current_user, "ENCRYPT", "DENIED", filename);
        return;
    }

    FILE* in = fopen(filename, "rb");
    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    fseek(in, 0, SEEK_SET);
    unsigned char* buf = malloc(size);
    fread(buf, 1, size, in);
    fclose(in);

    xor_cipher(buf, size, key);

    FILE* out = fopen(filename, "wb");
    fwrite(buf, 1, size, out);
    fclose(out);
    free(buf);

    files[idx].is_encrypted = 1;
    printf("[ENCRYPT] '%s' encrypted by '%s'.\n", filename, current_user);
    log_event(current_user, "ENCRYPT", "SUCCESS", filename);
}

void decrypt_secure_file(const char* filename, const char* key) {
    if (!require_login("DECRYPT")) return;
    int idx = find_file(filename);
    if (idx == -1 || !has_permission(&files[idx], 'r')) {
        printf("[DECRYPT] DENIED or file not found: '%s'.\n", filename);
        log_event(current_user, "DECRYPT", "DENIED", filename);
        return;
    }

    FILE* in = fopen(filename, "rb");
    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    fseek(in, 0, SEEK_SET);
    unsigned char* buf = malloc(size);
    fread(buf, 1, size, in);
    fclose(in);

    xor_cipher(buf, size, key);

    FILE* out = fopen(filename, "wb");
    fwrite(buf, 1, size, out);
    fclose(out);
    free(buf);

    files[idx].is_encrypted = 0;
    printf("[DECRYPT] '%s' decrypted by '%s'.\n", filename, current_user);
    log_event(current_user, "DECRYPT", "SUCCESS", filename);
}

/* ---------------- Demo ---------------- */

int main() {
    remove(LOG_FILE);

    printf("=========== SECURE FILE MANAGEMENT SYSTEM DEMO ===========\n\n");

    printf("--- User registration ---\n");
    register_user("alice", "correcthorse123", "staff");
    register_user("bob",   "hunter2",         "staff");
    register_user("carol", "letmein",         "guests");

    printf("\n--- Alice logs in and creates a file ---\n");
    login("alice", "correcthorse123");
    PermissionSet owner_rw = {1, 1, 0}, group_r = {1, 0, 0}, others_none = {0, 0, 0};
    create_secure_file("report.txt", "Confidential Q3 figures.", owner_rw, group_r, others_none);
    read_secure_file("report.txt");
    logout();

    printf("\n--- Bob (same group) logs in ---\n");
    login("bob", "hunter2");
    read_secure_file("report.txt");             // allowed: group read
    write_secure_file("report.txt", "hacked!");  // denied: group has no write
    logout();

    printf("\n--- Carol (different group) logs in ---\n");
    login("carol", "letmein");
    read_secure_file("report.txt");              // denied: others have no access
    logout();

    printf("\n--- Alice logs back in, encrypts, then deletes the file ---\n");
    login("alice", "correcthorse123");
    encrypt_secure_file("report.txt", "S3cretKey");
    decrypt_secure_file("report.txt", "S3cretKey");
    delete_secure_file("report.txt");
    logout();

    printf("\n--- Unauthenticated attempt (nobody logged in) ---\n");
    read_secure_file("report.txt"); // should be denied, no session

    printf("\n=========== FULL AUDIT LOG ===========\n");
    FILE* log = fopen(LOG_FILE, "r");
    char line[256];
    while (fgets(line, sizeof(line), log)) printf("%s", line);
    fclose(log);

    return 0;
}
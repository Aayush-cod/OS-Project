#include <stdio.h>
#include <time.h>

#define LOG_FILE "audit.log"

// Appends a single timestamped entry to the audit log.
// Format: [YYYY-MM-DD HH:MM:SS] user=<user> action=<action> result=<result> detail=<detail>
void log_event(const char* user, const char* action, const char* result, const char* detail) {
    FILE* log = fopen(LOG_FILE, "a"); // append mode - never overwrite past history
    if (log == NULL) {
        printf("Warning: could not write to audit log.\n");
        return;
    }

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    fprintf(log, "[%s] user=%-8s action=%-10s result=%-8s detail=%s\n",
            timestamp, user, action, result, detail);

    fclose(log);
}

// Prints the full contents of the audit log to the console
void print_audit_log() {
    FILE* log = fopen(LOG_FILE, "r");
    if (log == NULL) {
        printf("No audit log entries yet.\n");
        return;
    }

    char line[256];
    printf("--- Audit Log Contents ---\n");
    while (fgets(line, sizeof(line), log)) {
        printf("%s", line);
    }
    fclose(log);
}

int main() {
    remove(LOG_FILE); // start fresh for this demo run

    // Simulate a realistic sequence of security-relevant events across
    // the whole system (login attempts, file access, permission checks)
    log_event("alice", "LOGIN",      "SUCCESS", "correct credentials");
    log_event("alice", "CREATE",     "SUCCESS", "report.txt");
    log_event("bob",   "LOGIN",      "FAILED",  "incorrect password");
    log_event("bob",   "LOGIN",      "SUCCESS", "correct credentials on 2nd attempt");
    log_event("bob",   "READ",       "SUCCESS", "report.txt (group permission)");
    log_event("bob",   "WRITE",      "DENIED",  "report.txt (insufficient permissions)");
    log_event("carol", "READ",       "DENIED",  "report.txt (not owner or group)");
    log_event("alice", "ENCRYPT",    "SUCCESS", "salary.txt -> salary.txt.enc");
    log_event("mallory","DECRYPT",   "FAILED",  "salary.txt.enc (wrong key)");
    log_event("alice", "DELETE",     "SUCCESS", "report.txt");

    printf("Logged 10 events to '%s'.\n\n", LOG_FILE);
    print_audit_log();

    return 0;
}
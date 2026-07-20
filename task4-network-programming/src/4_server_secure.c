

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_MESSAGE_LEN 200   // reject anything suspiciously long
#define MAX_USERS 5

typedef struct {
    const char* username;
    const char* password;
} Credential;

// A small hardcoded "user database" for this demo (a real system would
// reuse Task 3's hashed-password authentication instead of plain text)
Credential valid_users[MAX_USERS] = {
    {"alice", "pass123"},
    {"bob",   "hunter2"},
};
int num_valid_users = 2;

// Checks provided credentials against the user list
int authenticate(const char* username, const char* password) {
    for (int i = 0; i < num_valid_users; i++) {
        if (strcmp(valid_users[i].username, username) == 0 &&
            strcmp(valid_users[i].password, password) == 0) {
            return 1;
        }
    }
    return 0;
}

// Validates a raw incoming message before any processing:
// rejects empty messages and messages exceeding MAX_MESSAGE_LEN.
// Returns 1 if valid, 0 if it should be rejected.
int validate_message(const char* message, int length) {
    if (length <= 0) return 0;
    if (length > MAX_MESSAGE_LEN) return 0;
    return 1;
}

int handle_message(const char* message, char* out_response, int* logged_in,
                    char* session_user) {
    char command[32] = {0};
    const char* colon = strchr(message, ':');

    if (colon == NULL) {
        strcpy(out_response, "ERROR:malformed message (expected COMMAND:PAYLOAD)");
        return 0;
    }

    int cmd_len = colon - message;
    if (cmd_len >= (int)sizeof(command)) { // guard against overlong command names
        strcpy(out_response, "ERROR:command too long");
        return 0;
    }
    strncpy(command, message, cmd_len);
    command[cmd_len] = '\0';
    const char* payload = colon + 1;

    // LOGIN is the only command allowed before authentication
    if (strcmp(command, "LOGIN") == 0) {
        char username[64], password[64];
        if (sscanf(payload, "%63[^:]:%63s", username, password) != 2) {
            strcpy(out_response, "ERROR:LOGIN requires username:password");
            return 0;
        }

        if (authenticate(username, password)) {
            *logged_in = 1;
            strcpy(session_user, username);
            sprintf(out_response, "LOGIN_OK:welcome %s", username);
        } else {
            strcpy(out_response, "LOGIN_FAILED:invalid credentials");
        }
        return 0;
    }

    // Every other command requires an authenticated session first
    if (!*logged_in) {
        strcpy(out_response, "ERROR:not authenticated - send LOGIN:user:pass first");
        return 0;
    }

    if (strcmp(command, "ECHO") == 0) {
        sprintf(out_response, "ECHO_REPLY:%s", payload);
    } else if (strcmp(command, "TIME") == 0) {
        time_t now = time(NULL);
        char* time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0';
        sprintf(out_response, "TIME_REPLY:%s", time_str);
    } else if (strcmp(command, "QUIT") == 0) {
        strcpy(out_response, "BYE:closing connection");
        return 1;
    } else {
        sprintf(out_response, "ERROR:unknown command '%s'", command);
    }

    return 0;
}

void handle_client(int client_fd, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int logged_in = 0;
    char session_user[64] = "(unauthenticated)";

    printf("[PID %d] Handling client %s:%d\n",
           getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    while (1) {
        int bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read <= 0) {
            printf("[PID %d] Client disconnected.\n", getpid());
            break;
        }
        buffer[bytes_read] = '\0';

        // Validate BEFORE processing - untrusted input from the network
        if (!validate_message(buffer, bytes_read)) {
            printf("[PID %d] REJECTED invalid message (length %d)\n", getpid(), bytes_read);
            const char* err = "ERROR:invalid message (empty or too long)";
            send(client_fd, err, strlen(err), 0);
            continue;
        }

        printf("[PID %d] [%s] Received: \"%s\"\n", getpid(), session_user, buffer);

        int should_quit = handle_message(buffer, response, &logged_in, session_user);
        send(client_fd, response, strlen(response), 0);
        printf("[PID %d] [%s] Sent: \"%s\"\n", getpid(), session_user, response);

        if (should_quit) break;
    }

    close(client_fd);
    printf("[PID %d] Connection closed, child process exiting.\n", getpid());
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    signal(SIGCHLD, SIG_IGN);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    printf("[PID %d] Secure server listening on port %d...\n", getpid(), PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&address, &addr_len);
        if (client_fd < 0) continue;

        pid_t pid = fork();
        if (pid == 0) {
            close(server_fd);
            handle_client(client_fd, address);
            exit(0);
        } else {
            close(client_fd);
        }
    }

    return 0;
}
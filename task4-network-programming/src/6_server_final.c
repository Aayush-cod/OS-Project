#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_MESSAGE_LEN 200
#define MAX_USERS 5

typedef struct {
    const char* username;
    const char* password;
} Credential;

Credential valid_users[MAX_USERS] = {
    {"alice", "pass123"},
    {"bob",   "hunter2"},
};
int num_valid_users = 2;

int authenticate(const char* username, const char* password) {
    for (int i = 0; i < num_valid_users; i++) {
        if (strcmp(valid_users[i].username, username) == 0 &&
            strcmp(valid_users[i].password, password) == 0) {
            return 1;
        }
    }
    return 0;
}

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
    if (cmd_len >= (int)sizeof(command)) {
        strcpy(out_response, "ERROR:command too long");
        return 0;
    }
    strncpy(command, message, cmd_len);
    command[cmd_len] = '\0';
    const char* payload = colon + 1;

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

// Safely sends a response, checking for errors instead of assuming success.
// Returns 1 if the send succeeded, 0 if it failed (e.g. client already gone).
int safe_send(int client_fd, const char* response) {
    int result = send(client_fd, response, strlen(response), 0);
    if (result < 0) {
        printf("[PID %d] send() failed (errno %d: %s) - client likely gone\n",
               getpid(), errno, strerror(errno));
        return 0;
    }
    return 1;
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

        if (bytes_read == 0) {
            // Clean disconnect: client closed the connection normally,
            // without sending QUIT (e.g. it crashed or was killed)
            printf("[PID %d] Client disconnected abruptly (no QUIT received).\n", getpid());
            break;
        } else if (bytes_read < 0) {
            // Actual network error (not just a normal disconnect)
            printf("[PID %d] recv() error (errno %d: %s)\n", getpid(), errno, strerror(errno));
            break;
        }

        buffer[bytes_read] = '\0';

        if (!validate_message(buffer, bytes_read)) {
            printf("[PID %d] REJECTED invalid message (length %d)\n", getpid(), bytes_read);
            safe_send(client_fd, "ERROR:invalid message (empty or too long)");
            continue;
        }

        printf("[PID %d] [%s] Received: \"%s\"\n", getpid(), session_user, buffer);

        int should_quit = handle_message(buffer, response, &logged_in, session_user);

        if (!safe_send(client_fd, response)) {
            break; // client gone, no point continuing this connection
        }
        printf("[PID %d] [%s] Sent: \"%s\"\n", getpid(), session_user, response);

        if (should_quit) break;
    }

    close(client_fd);
    printf("[PID %d] Connection cleanup complete, child process exiting.\n\n", getpid());
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    // Prevent the server from being killed by SIGPIPE when writing to
    // a socket the client has already closed - send() will instead
    // just return -1, which we already handle via safe_send()
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN); // auto-reap finished child processes

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(1);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(1);
    }

    printf("[PID %d] Final server listening on port %d...\n\n", getpid(), PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&address, &addr_len);
        if (client_fd < 0) {
            // Don't let one bad accept() bring down the whole server
            printf("[PID %d] accept() failed (errno %d: %s), continuing...\n",
                   getpid(), errno, strerror(errno));
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            // fork() failed (e.g. process limit reached) - reject this
            // one client gracefully rather than crashing the server
            printf("[PID %d] fork() failed (errno %d: %s), rejecting this client.\n",
                   getpid(), errno, strerror(errno));
            close(client_fd);
        } else if (pid == 0) {
            close(server_fd);
            handle_client(client_fd, address);
            exit(0);
        } else {
            close(client_fd);
        }
    }

    return 0;
}
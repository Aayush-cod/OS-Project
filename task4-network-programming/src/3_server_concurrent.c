#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Parses "COMMAND:PAYLOAD" and handles it, writing the response into
// out_response. Returns 0 normally, or 1 if the client sent QUIT.
int handle_message(const char* message, char* out_response) {
    char command[32] = {0};
    const char* colon = strchr(message, ':');

    if (colon == NULL) {
        strcpy(out_response, "ERROR:malformed message (expected COMMAND:PAYLOAD)");
        return 0;
    }

    int cmd_len = colon - message;
    strncpy(command, message, cmd_len);
    command[cmd_len] = '\0';
    const char* payload = colon + 1;

    if (strcmp(command, "ECHO") == 0) {
        sprintf(out_response, "ECHO_REPLY:%s", payload);
    } else if (strcmp(command, "TIME") == 0) {
        time_t now = time(NULL);
        char* time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0'; // strip trailing newline from ctime
        sprintf(out_response, "TIME_REPLY:%s", time_str);
    } else if (strcmp(command, "QUIT") == 0) {
        strcpy(out_response, "BYE:closing connection");
        return 1;
    } else {
        sprintf(out_response, "ERROR:unknown command '%s'", command);
    }

    return 0;
}

// Handles the full conversation with ONE client, in a child process
void handle_client(int client_fd, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    printf("[PID %d] Handling client %s:%d\n",
           getpid(), inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    while (1) {
        int bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_read <= 0) {
            printf("[PID %d] Client disconnected.\n", getpid());
            break;
        }
        buffer[bytes_read] = '\0';
        printf("[PID %d] Received: \"%s\"\n", getpid(), buffer);

        int should_quit = handle_message(buffer, response);
        send(client_fd, response, strlen(response), 0);
        printf("[PID %d] Sent: \"%s\"\n", getpid(), response);

        if (should_quit) break;
    }

    close(client_fd);
    printf("[PID %d] Connection closed, child process exiting.\n", getpid());
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);

    // Automatically reap finished child processes so they don't
    // linger as zombies (SIG_IGN on SIGCHLD does this on POSIX systems)
    signal(SIGCHLD, SIG_IGN);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    printf("[PID %d] Concurrent server listening on port %d...\n", getpid(), PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&address, &addr_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            close(client_fd);
        } else if (pid == 0) {
            // Child process: handles this one client exclusively
            close(server_fd); // child doesn't need the listening socket
            handle_client(client_fd, address);
            exit(0);
        } else {
            // Parent process: doesn't need this client's socket,
            // immediately loops back to accept the next connection
            close(client_fd);
        }
    }

    return 0;
}
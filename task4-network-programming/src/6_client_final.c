#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

int sock_fd;

void send_and_receive(const char* message) {
    char buffer[BUFFER_SIZE];
    send(sock_fd, message, strlen(message), 0);
    printf("Sent: \"%s\"\n", message);

    int bytes_read = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read <= 0) {
        printf("(no response - connection closed)\n\n");
        return;
    }
    buffer[bytes_read] = '\0';
    printf("Received: \"%s\"\n\n", buffer);
}

int main() {
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);

    if (connect(sock_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("connect failed - is the server running?");
        exit(1);
    }
    printf("Connected to final server.\n\n");

    printf("--- Login ---\n");
    send_and_receive("LOGIN:bob:hunter2");

    printf("--- Malformed message (no colon at all) ---\n");
    send_and_receive("this is not a valid message");

    printf("--- Normal commands ---\n");
    send_and_receive("ECHO:testing error handling");
    send_and_receive("TIME:");

    printf("--- Quit cleanly ---\n");
    send_and_receive("QUIT:");

    close(sock_fd);
    printf("Connection closed normally.\n");

    return 0;
}
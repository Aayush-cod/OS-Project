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
    printf("Connected to secure server.\n\n");

    printf("--- Attempt 1: command before login ---\n");
    send_and_receive("ECHO:should be rejected");

    printf("--- Attempt 2: login with WRONG password ---\n");
    send_and_receive("LOGIN:alice:wrongpassword");

    printf("--- Attempt 3: login with CORRECT credentials ---\n");
    send_and_receive("LOGIN:alice:pass123");

    printf("--- Attempt 4: commands now that we're authenticated ---\n");
    send_and_receive("ECHO:now it works");
    send_and_receive("TIME:");
    send_and_receive("QUIT:");

    close(sock_fd);
    printf("Connection closed.\n");

    return 0;
}
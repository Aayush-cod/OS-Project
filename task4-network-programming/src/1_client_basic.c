#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1" // localhost - server and client on same machine

int main() {
    int sock_fd;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];

    // Step 1: create a TCP socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket failed");
        exit(1);
    }

    // Step 2: define the server's address and port to connect to
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("invalid server address");
        exit(1);
    }

    // Step 3: connect to the server (this blocks until connected or fails)
    if (connect(sock_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("connect failed - is the server running?");
        exit(1);
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, PORT);

    // Step 4: send a message to the server
    const char* message = "Hello from client!";
    send(sock_fd, message, strlen(message), 0);
    printf("Sent to server: \"%s\"\n", message);

    // Step 5: wait for and receive the server's reply
    int bytes_read = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
    buffer[bytes_read] = '\0';
    printf("Received from server: \"%s\"\n", buffer);

    // Step 6: close the connection
    close(sock_fd);
    printf("Connection closed.\n");

    return 0;
}
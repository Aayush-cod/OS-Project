#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

void send_and_receive(int sock_fd, const char* message, const char* client_name) {
    char buffer[BUFFER_SIZE];
    send(sock_fd, message, strlen(message), 0);
    printf("[%s] Sent: \"%s\"\n", client_name, message);

    int bytes_read = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
    buffer[bytes_read] = '\0';
    printf("[%s] Received: \"%s\"\n", client_name, buffer);
}

int main(int argc, char* argv[]) {
    const char* client_name = (argc > 1) ? argv[1] : "client";

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);

    if (connect(sock_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("connect failed - is the server running?");
        exit(1);
    }
    printf("[%s] Connected to server.\n", client_name);

    char echo_msg[64];
    sprintf(echo_msg, "ECHO:Hello from %s", client_name);

    send_and_receive(sock_fd, echo_msg, client_name);
    send_and_receive(sock_fd, "TIME:", client_name);
    send_and_receive(sock_fd, "QUIT:", client_name);

    close(sock_fd);
    printf("[%s] Connection closed.\n", client_name);

    return 0;
}
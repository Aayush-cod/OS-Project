#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    char buffer[BUFFER_SIZE];

    // Step 1: create a TCP socket (AF_INET = IPv4, SOCK_STREAM = TCP)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }

    // Allow the port to be reused immediately after the program exits
    // (otherwise re-running quickly can fail with "address already in use")
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Step 2: define the address to bind to - any local interface, our PORT
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT); // host-to-network byte order

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(1);
    }

    // Step 3: mark the socket as passive (ready to accept connections),
    // with a backlog of 5 pending connections
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    // Step 4: block here until a client connects
    client_fd = accept(server_fd, (struct sockaddr*)&address, &addr_len);
    if (client_fd < 0) {
        perror("accept failed");
        exit(1);
    }

    printf("Client connected from %s:%d\n",
           inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    // Step 5: receive a message from the client
    int bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    buffer[bytes_read] = '\0';
    printf("Received from client: \"%s\"\n", buffer);

    // Step 6: send a reply
    const char* reply = "Hello from server!";
    send(client_fd, reply, strlen(reply), 0);
    printf("Sent reply: \"%s\"\n", reply);

    // Step 7: close both sockets cleanly
    close(client_fd);
    close(server_fd);
    printf("Connection closed. Server shutting down.\n");

    return 0;
}
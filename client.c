#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define NI_MAXHOST 1025
#define NI_MAXSERV 32

void *server_message_handler(void *arg) {
    int fd = *((int *)arg);
    ssize_t bytes_received;
    char received_message[1024];

    while(1) {
        if ((bytes_received = recv(fd, received_message, sizeof(received_message) - 1, 0)) > 0) {
            received_message[bytes_received] = '\0';
            printf("%s\n", received_message);
        } else if (bytes_received == 0) {
            // Connection closed by server
            printf("Server closed the connection\n");
            break;
        } else {
            perror("recv error:");
            break;
        }
    }
    return NULL;
}

static void client(const char *server_ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket error:");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((short)port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        perror("inet_pton error:");
        close(fd);
        return;
    }

    // Connect to server
    if (connect(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("connect error:");
        close(fd);
        return;
    }

    char name[50];
    printf("Please enter a username:\n");

    if (fgets(name, sizeof(name), stdin) == NULL) {
        printf("Failed to read input.\n");
        close(fd);
        return;
    }

    // Remove newline character from name
    name[strcspn(name, "\n")] = 0;

    if (send(fd, name, strlen(name), 0) == -1) {
        perror("send error:");
        close(fd);
        return;
    }

    printf("Entering server with username: %s\n", name);

    // Create a thread to receive messages from the server
    pthread_t tid;

    if (pthread_create(&tid, NULL, server_message_handler, &fd) != 0) {
        perror("pthread_create error:");
        close(fd);
        return;
    }

    char message[1024];
    while (1) {
        if (fgets(message, sizeof(message), stdin) == NULL) {
            printf("Failed to read input.\n");
            break;
        }

        // Remove newline character from message
        message[strcspn(message, "\n")] = 0;

        if (send(fd, message, strlen(message), 0) == -1) {
            perror("send error:");
            break;
        }
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "client") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Usage: %s client <server_ip> <port>\n", argv[0]);
            return -1;
        }

        const char *server_ip = argv[2];
        int port;
        sscanf(argv[3], "%d", &port);

        client(server_ip, port);
    } else {
        fprintf(stderr, "Invalid arguments\n");
        return -1;
    }
    return 0;
}
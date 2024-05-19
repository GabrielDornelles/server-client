#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#  define NI_MAXHOST      1025
#  define NI_MAXSERV      32


void *server_message_handler(void *arg){
    int fd = *((int *)arg);
    ssize_t bytes_received;
    char received_message[1024];

    while(1){
        if ((bytes_received = recv(fd, received_message, sizeof(received_message), 0)) > 0){
            received_message[bytes_received] = '\0';
            printf("%s\n", received_message);
        }
    }
    return NULL;

}

static void client(int port) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons((short) port);

    // connect to local machine at specified port
    char addrstr[NI_MAXHOST + NI_MAXSERV + 1];
    snprintf(addrstr, sizeof(addrstr), "127.0.0.1:%d", port);

    // parse into address
    inet_pton(AF_INET, addrstr, &addr.sin_addr);

    // connect to server
    if (connect(fd, (struct sockaddr*) &addr, sizeof(addr))) {
        perror("connect error:");
        return;
    } 

    char name[50];
    printf("Please enter a username:\n");
    
    if (scanf("%[^\n]%*c", name) != 1) {
        printf("Failed to read input.\n");
    }
    
    name[sizeof(name)] = '\0';

    if (send(fd, name, strlen(name), 0) == -1) {
        perror("send error:");
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
       
        if (scanf("%[^\n]%*c", message) != 1) {
            printf("Failed to read input.\n");
            break;
        }
       
        if (send(fd, message, strlen(message), 0) == -1) {
            perror("send error:");
            break;
        }
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc > 1 && !strcmp(argv[1], "client")) {
        if (argc != 3) {
            fprintf(stderr, "not enough args!");
            return -1;
        }

        int port;
        sscanf(argv[2], "%d", &port);

        client(port);
    } 
    return 0;
}
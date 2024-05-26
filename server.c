#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#define MAX_CLIENTS 1024

int num_of_connected_clients = 0;

struct ClientData {
    char name[50];
	int sockID; //cfd
};

struct ThreadArgs {
    int cfd;
    struct ClientData *client_data;
    int client_array_id;
    int* num_of_connected_clients;
};

void log_message(int level, const char *format, ...) {
    const char *level_strings[] = { "INFO", "WARN", "ERROR" };
    va_list args;
    time_t now;
    char buf[20];

    time(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(stdout, "[%s][%s]: ", buf, level_strings[level]);

    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}


void send_messages(char *message, char *client_name, struct ClientData *client_data, int * num_clients) {
    char formatted_message[1074];

    for(int i = 0; i < *num_clients; i++){
        size_t len = strlen(message);

        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }

        snprintf(formatted_message, sizeof(formatted_message), "(%s): %s", client_name, message);
        if (send(client_data[i].sockID, formatted_message, strlen(formatted_message), 0) == -1) {
            perror("send error:");
            break;
        }
    }
}

void *client_handler(void *arg) {
    struct ThreadArgs *thread_args = (struct ThreadArgs *)arg;
    struct ClientData *client_data = thread_args->client_data;
    int cfd = thread_args->cfd;
    int *num_of_connected_clients = thread_args->num_of_connected_clients;
    int client_array_id = thread_args->client_array_id;

    char message[1024];
    ssize_t bytes_received;

    while ((bytes_received = recv(cfd, message, sizeof(message) - 1, 0)) > 0) {
        message[bytes_received] = '\0';

        if (bytes_received > 1){
            send_messages(message, client_data[client_array_id].name, client_data, num_of_connected_clients);
            log_message(0, "Sent message: '%s' from %s\n", message, client_data[client_array_id].name);
        }
    }
        

    if (bytes_received == -1) {
        perror("recv error:");
    }

    // Close client sockets
    for (int i = 0; i < *num_of_connected_clients; i++) {
        close(client_data[i].sockID);
    }

    return NULL;
}

static int server() {
    int fd = socket(PF_INET, SOCK_STREAM, 0);

    if (fd == -1) {
        perror("socket error:");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8000);

    if (bind(fd, (struct sockaddr*) &addr, sizeof(addr))) {
        perror("bind error:");
        close(fd);
        return -1;
    }

    if (listen(fd, MAX_CLIENTS) == -1) {
        perror("listen error:");
        close(fd);
        return -1;
    }

    log_message(0, "Server is up on port %d\n", (int) ntohs(addr.sin_port));
    
    int client_count = 0;
    struct ClientData client_info[MAX_CLIENTS];
    
    while (1) {

        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(fd, (struct sockaddr*) &client_addr, &client_addr_len);
       
        if (client_fd == -1) {
            perror("accept error:");
            close(fd);
            return -1;
        }

        log_message(0, "Got connection with fd: %d, waiting for username\n", client_fd);

        char client_name[50];
        memset(client_name, 0, sizeof(client_name));
        ssize_t name_bytes_received = recv(client_fd, client_name, 50, 0);
        client_name[name_bytes_received] = '\0';
        log_message(0, "Received user with username: %s\n", client_name);

        if (name_bytes_received <= 0) {
            perror("recv client name error:");
            close(client_fd);
            continue;
        }
        
        strncpy(client_info[client_count].name, client_name, sizeof(client_info[client_count].name) - 1);
        client_info[client_count].sockID = client_fd;
    
        struct ThreadArgs args;

        args.cfd = client_fd;
        args.client_data = client_info;
        args.client_array_id = client_count;
        args.num_of_connected_clients = &num_of_connected_clients;

        pthread_t tid;

        if (pthread_create(&tid, NULL, client_handler, &args) != 0) {
            perror("pthread_create error:");
            close(client_fd);
            close(fd);
            return -1;
        }


        client_count++;
        num_of_connected_clients = client_count;

        pthread_detach(tid);
    }

    close(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    server();
    return 0;
}

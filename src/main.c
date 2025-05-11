#include "../include/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h> 

void handle_signal(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    client_list.running = false;
}

int main() {
    init_server();
    int server_socket = create_server_socket(PORT);
    printf("Server started on port %d\nWaiting for connections...\n", PORT);
    while (client_list.running) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        
        if (!client_list.running) break;
        
        if (client_socket < 0) {
            if (errno != EINTR) {
                perror("accept failed");
            }
            continue;
        }
        Client* new_client = calloc(1, sizeof(Client));
        if (!new_client) {
            perror("malloc failed");
            close(client_socket);
            continue;
        }
        new_client->socket = client_socket;
        new_client->should_free = true;
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, new_client) != 0) {
            perror("pthread_create failed");
            close(client_socket);
            free(new_client);
            continue;
        }
        new_client->thread_id = thread;
        pthread_detach(thread);
    }
    shutdown_server();
    close(server_socket);
    return 0;
}
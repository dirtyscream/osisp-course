#include "general.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void* handle_client(void*);

int main() {
    init_socket_api();

    socket_t server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close_socket(server_socket);
        return 1;
    }

    if (listen(server_socket, BACKLOG) < 0) {
        perror("listen");
        close_socket(server_socket);
        return 1;
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        socket_t* client_socket = malloc(sizeof(socket_t));
        if (!client_socket) continue;

        *client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (*client_socket < 0) {
            perror("accept");
            free(client_socket);
            continue;
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client_socket) != 0) {
            perror("pthread_create");
            close_socket(*client_socket);
            free(client_socket);
        } else {
            pthread_detach(thread);
        }
    }

    close_socket(server_socket);
    cleanup_socket_api();
    return 0;
}

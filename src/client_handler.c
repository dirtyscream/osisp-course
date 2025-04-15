#include "general.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* handle_client(void* arg) {
    socket_t client_socket = *(socket_t*)arg;
    free(arg);

    char buffer[MAX_BUFFER];
    int received;

    while ((received = recv(client_socket, buffer, MAX_BUFFER, 0)) > 0) {
        send(client_socket, buffer, received, 0); 
    }

    printf("Client disconnected\n");
    close_socket(client_socket);
    return NULL;
}

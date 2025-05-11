#include "../include/server.h"
#include "../include/chat.h"
#include "../include/auth.h"
#include "../include/commands.h"
#include "../include/file_transfer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

ClientList client_list = {0};
pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_server(void) {
    client_list.count = 0;
    client_list.running = true;
    pthread_mutex_init(&client_list.mutex, NULL);
    
    signal(SIGPIPE, SIG_IGN);
}

void shutdown_server() {
    pthread_mutex_lock(&client_list.mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_list.clients[i] != NULL) {
            printf("Shutting down client %s\n", client_list.clients[i]->username);
            if (client_list.clients[i]->socket > 0) {
                shutdown(client_list.clients[i]->socket, SHUT_RDWR);
                close(client_list.clients[i]->socket);
            }
            
            free(client_list.clients[i]);
            client_list.clients[i] = NULL;
        }
    }
    client_list.count = 0;
    pthread_mutex_unlock(&client_list.mutex);
}

void add_client(Client* client) {
    pthread_mutex_lock(&client_list.mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_list.clients[i] && client_list.clients[i]->socket == client->socket) {
            pthread_mutex_unlock(&client_list.mutex);
            return;
        }
    }
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_list.clients[i] == NULL) {
            client_list.clients[i] = client;
            client_list.count++;
            printf("Client %s added to slot %d\n", client->username, i);
            break;
        }
    }
    
    pthread_mutex_unlock(&client_list.mutex);
}

void remove_client(int socket) {
    pthread_mutex_lock(&client_list.mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_list.clients[i] != NULL && 
            client_list.clients[i]->socket == socket) {
            
            printf("Removing client %s (socket %d)\n", 
                  client_list.clients[i]->username, socket);
            client_list.clients[i]->should_free = false;
            free(client_list.clients[i]);
            client_list.clients[i] = NULL;
            client_list.count--;
            break;
        }
    }
    
    pthread_mutex_unlock(&client_list.mutex);
}


void broadcast_message(const char* message, int sender_socket) {
    pthread_mutex_lock(&client_list.mutex);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_list.clients[i] != NULL && 
            client_list.clients[i]->socket != sender_socket &&
            client_list.clients[i]->authenticated) {
            
            
            if (send(client_list.clients[i]->socket, message, strlen(message), MSG_NOSIGNAL) < 0) {
                
                printf("Failed to send to client %s, marking for removal\n", 
                      client_list.clients[i]->username);
                client_list.clients[i]->socket = -1;
            }
        }
    }
    
    pthread_mutex_unlock(&client_list.mutex);
}

void send_message(int socket, const char* message) {
    send(socket, message, strlen(message), 0);
}

ssize_t receive_message(int socket, char* buffer, size_t size) {
    memset(buffer, 0, size);
    ssize_t bytes_received = recv(socket, buffer, size - 1, 0);
    
    if (bytes_received <= 0) {
        return bytes_received; 
    }
    
    
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return bytes_received;
}

int create_server_socket(int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error bind");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_socket, 10) < 0) {
        perror("Error listen");
        exit(EXIT_FAILURE);
    }
    
    return server_socket;
}
bool authenticate_client(Client* client) {
    char buffer[BUFFER_SIZE];
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];

    send_message(client->socket, "Enter your username: ");
    if (receive_message(client->socket, buffer, BUFFER_SIZE) <= 0) {
        return false;
    }
    strncpy(username, buffer, MAX_USERNAME_LEN);

    send_message(client->socket, "Enter your password: ");
    if (receive_message(client->socket, buffer, BUFFER_SIZE) <= 0) {
        return false;
    }
    strncpy(password, buffer, MAX_PASSWORD_LEN);

    pthread_mutex_lock(&db_mutex);
    bool auth_result = authenticate_user(username, password);
    pthread_mutex_unlock(&db_mutex);

    if (!auth_result) {
        send_message(client->socket, "Authentication failed. Disconnecting...\n");
        return false;
    }

    strncpy(client->username, username, MAX_USERNAME_LEN);
    client->authenticated = true;
    
    add_client(client);
    printf("Client %s authenticated successfully\n", username);
    return true;
}


void* handle_client(void* arg) {
    Client* client = (Client*)arg;
    char buffer[BUFFER_SIZE];
    client->should_free = true;
    
    
    struct timeval tv;
    tv.tv_sec = 30;  
    tv.tv_usec = 0;
    setsockopt(client->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    
    if (!authenticate_client(client)) {
        goto cleanup;
    }
    
    
    while (client->authenticated) {
        
        send_message(client->socket, "\nEnter mode:\n");
        send_message(client->socket, "1 - Chat\n");
        send_message(client->socket, "2 - SSH-like console\n");
        send_message(client->socket, "3 - File transfer\n");
        send_message(client->socket, "0 - Exit\n");
        send_message(client->socket, "Input: ");
        
        
        ssize_t bytes_received = recv(client->socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Client %s disconnected\n", client->username);
            break;
        }
        buffer[bytes_received] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0';  
        
        int mode = atoi(buffer);
        
        switch (mode) {
            case 1:
                handle_chat_mode(client->socket, client->username);
                break;
            case 2:
                handle_command_mode(client->socket);
                break;
            case 3:
                handle_file_transfer_mode(client->socket);
                break;
            case 0:
                send_message(client->socket, "Goodbye!\n");
                client->authenticated = false;
                break;
            default:
                send_message(client->socket, "Incorrect mode\n");
                break;
        }
        
        if (!client->authenticated) {
            break;
        }
    }

cleanup:
    if (client->authenticated) {
        remove_client(client->socket);
    }
    if (client->socket > 0) {
        close(client->socket);
        client->socket = -1;
    }
    if (client->should_free) {
        free(client);
    }
    return NULL;
}

#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50

typedef struct {
    int socket;
    char username[MAX_USERNAME_LEN];
    bool authenticated;
} Client;

typedef struct {
    Client* clients[MAX_CLIENTS];
    int count;
    pthread_mutex_t mutex;
} ClientList;

// Объявляем глобальные переменные
extern ClientList client_list;
extern pthread_mutex_t db_mutex;

// Объявляем функции
void init_server();
void* handle_client(void* arg);
void broadcast_message(const char* message, int sender_socket);
void add_client(Client* client);
void remove_client(int socket);
void send_message(int socket, const char* message);
void receive_message(int socket, char* buffer, size_t size);
int create_server_socket(int port);

#endif
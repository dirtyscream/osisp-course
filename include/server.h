#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50

#if defined(__APPLE__) || defined(__MACH__)
    #define TCP_KEEPALIVE_OPTIONS 1
    #define TCP_KEEPIDLE TCP_KEEPALIVE
#elif defined(__linux__)
    #define TCP_KEEPALIVE_OPTIONS 1
#endif


typedef struct {
    int socket;
    char username[MAX_USERNAME_LEN];
    bool authenticated;
    pthread_t thread_id;
    bool should_free; 
} Client;

typedef struct {
    Client* clients[MAX_CLIENTS];
    int count;
    pthread_mutex_t mutex;
    volatile bool running;
} ClientList;


extern ClientList client_list;
extern pthread_mutex_t db_mutex;


void init_server(void);
void shutdown_server(void);
void* handle_client(void* arg);
void add_client(Client* client);
void remove_client(int socket);
void broadcast_message(const char* message, int sender_socket);
void send_message(int socket, const char* message);
ssize_t receive_message(int socket, char* buffer, size_t size);
int create_server_socket(int port);

#endif
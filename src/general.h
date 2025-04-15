#ifndef GENERAL_H
#define GENERAL_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET socket_t;
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
typedef int socket_t;
#endif

#define MAX_BUFFER 1024
#define PORT 12345
#define BACKLOG 10

void init_socket_api();
void cleanup_socket_api();
void close_socket(socket_t sock);

#endif

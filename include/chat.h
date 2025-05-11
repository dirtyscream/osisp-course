#ifndef CHAT_H
#define CHAT_H

#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

void handle_chat_mode(int client_socket, const char* username);
void send_chat_history(int client_socket);
void add_to_chat_history(const char* username, const char* message);

#endif

#include "../include/chat.h"
#include "../include/server.h"
#include <time.h>

#define MAX_CHAT_HISTORY 100

typedef struct {
    char message[BUFFER_SIZE];
    char username[MAX_USERNAME_LEN];
    time_t timestamp;
} ChatMessage;

ChatMessage chat_history[MAX_CHAT_HISTORY];
int chat_history_count = 0;
pthread_mutex_t chat_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_to_chat_history(const char* username, const char* message) {
    pthread_mutex_lock(&chat_mutex);
    
    if (chat_history_count >= MAX_CHAT_HISTORY) {
        for (int i = 1; i < MAX_CHAT_HISTORY; i++) {
            chat_history[i-1] = chat_history[i];
        }
        chat_history_count--;
    }
    
    strncpy(chat_history[chat_history_count].username, username, MAX_USERNAME_LEN);
    strncpy(chat_history[chat_history_count].message, message, BUFFER_SIZE);
    chat_history[chat_history_count].timestamp = time(NULL);
    chat_history_count++;
    
    pthread_mutex_unlock(&chat_mutex);
}

void send_chat_history(int client_socket) {
    pthread_mutex_lock(&chat_mutex);
    
    send_message(client_socket, "\n--- История чата ---");
    for (int i = 0; i < chat_history_count; i++) {
        char formatted[BUFFER_SIZE + MAX_USERNAME_LEN + 50];
        struct tm* timeinfo = localtime(&chat_history[i].timestamp);
        strftime(formatted, sizeof(formatted), "[%H:%M:%S] ", timeinfo);
        strcat(formatted, chat_history[i].username);
        strcat(formatted, ": ");
        strcat(formatted, chat_history[i].message);
        
        send_message(client_socket, formatted);
    }
    send_message(client_socket, "-------------------\n");
    
    pthread_mutex_unlock(&chat_mutex);
}

void handle_chat_mode(int client_socket, const char* username) {
    char buffer[BUFFER_SIZE];
    
    send_message(client_socket, "Вы вошли в режим чата. Для выхода введите /exit");
    send_chat_history(client_socket);
    
    while (1) {
        receive_message(client_socket, buffer, BUFFER_SIZE);
        
        if (strcmp(buffer, "/exit") == 0) {
            break;
        }
        
        add_to_chat_history(username, buffer);
        broadcast_message(buffer, client_socket);
    }
    
    send_message(client_socket, "Вы вышли из режима чата");
}
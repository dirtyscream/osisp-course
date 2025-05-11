#include "../include/chat.h"
#include "../include/server.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

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

void handle_chat_mode(int client_socket, const char* username) {
    char buffer[BUFFER_SIZE];
    send_message(client_socket, "\n=== CHAT MODE ===\n");
    send_message(client_socket, "Logged in as: ");
    send_message(client_socket, username);
    send_message(client_socket, "\nType /exit to quit\n");
    send_message(client_socket, "------------------\n");

    while (1) {
        
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Client %s disconnected\n", username);
            break;
        }
        buffer[strcspn(buffer, "\n")] = '\0';
        
        if (strcmp(buffer, "/exit") == 0) {
            break;
        }
        if (strlen(buffer) > 0) {
            add_to_chat_history(username, buffer);
            char broadcast_msg[BUFFER_SIZE + MAX_USERNAME_LEN + 10];
            snprintf(broadcast_msg, sizeof(broadcast_msg), "%s: %s\n", username, buffer);
            broadcast_message(broadcast_msg, client_socket);
        }
    }

    send_message(client_socket, "\nGoodbye!\n\n");
    close(client_socket);
}
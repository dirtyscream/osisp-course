#include "../include/server.h"
#include "../include/auth.h"
#include "../include/chat.h"
#include "../include/commands.h"
#include "../include/file_transfer.h"

ClientList client_list;
pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_server() {
    client_list.count = 0;
    pthread_mutex_init(&client_list.mutex, NULL);
    init_database();
}

void add_client(Client* client) {
    pthread_mutex_lock(&client_list.mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_list.clients[i] == NULL) {
            client_list.clients[i] = client;
            client_list.count++;
            break;
        }
    }
    pthread_mutex_unlock(&client_list.mutex);
}

void remove_client(int socket) {
    pthread_mutex_lock(&client_list.mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_list.clients[i] != NULL && client_list.clients[i]->socket == socket) {
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
            send_message(client_list.clients[i]->socket, message);
        }
    }
    pthread_mutex_unlock(&client_list.mutex);
}

void send_message(int socket, const char* message) {
    send(socket, message, strlen(message), 0);
}

void receive_message(int socket, char* buffer, size_t size) {
    ssize_t bytes_received = recv(socket, buffer, size - 1, 0);
    if (bytes_received <= 0) {
        buffer[0] = '\0';
    } else {
        buffer[bytes_received] = '\0';
        if (buffer[bytes_received - 1] == '\n') {
            buffer[bytes_received - 1] = '\0';
        }
    }
}

int create_server_socket(int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Ошибка bind");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_socket, 10) < 0) {
        perror("Ошибка listen");
        exit(EXIT_FAILURE);
    }
    
    return server_socket;
}

void* handle_client(void* arg) {
    Client* client = (Client*)arg;
    char buffer[BUFFER_SIZE];
    
    send_message(client->socket, "Введите ваш логин:");
    receive_message(client->socket, buffer, BUFFER_SIZE);
    char username[MAX_USERNAME_LEN];
    strncpy(username, buffer, MAX_USERNAME_LEN);
    
    send_message(client->socket, "Введите ваш пароль:");
    receive_message(client->socket, buffer, BUFFER_SIZE);
    char password[MAX_PASSWORD_LEN];
    strncpy(password, buffer, MAX_PASSWORD_LEN);
    
    pthread_mutex_lock(&db_mutex);
    bool auth_result = authenticate_user(username, password);
    pthread_mutex_unlock(&db_mutex);
    
    if (!auth_result) {
        send_message(client->socket, "Ошибка аутентификации. Соединение закрыто.");
        close(client->socket);
        free(client);
        return NULL;
    }
    
    strncpy(client->username, username, MAX_USERNAME_LEN);
    client->authenticated = true;
    add_client(client);
    
    send_message(client->socket, "Аутентификация успешна. Выберите режим работы:\n"
                  "1. Чат\n"
                  "2. Выполнение команд\n"
                  "3. Передача файлов");
    
    receive_message(client->socket, buffer, BUFFER_SIZE);
    int mode = atoi(buffer);
    
    switch (mode) {
        case 1:
            handle_chat_mode(client->socket, username);
            break;
        case 2:
            handle_command_mode(client->socket);
            break;
        case 3:
            handle_file_transfer_mode(client->socket);
            break;
        default:
            send_message(client->socket, "Неверный режим работы");
            break;
    }
    
    remove_client(client->socket);
    close(client->socket);
    free(client);
    return NULL;
}
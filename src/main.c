#include "../include/server.h"
#include "../include/auth.h"
#include "../include/chat.h"
#include "../include/commands.h"
#include "../include/file_transfer.h"

int main() {
    init_server();
    int server_socket = create_server_socket(PORT);
    
    printf("Сервер запущен на порту %d\n", PORT);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        
        if (client_socket < 0) {
            perror("Ошибка accept");
            continue;
        }
        
        Client* new_client = malloc(sizeof(Client));
        new_client->socket = client_socket;
        new_client->authenticated = false;
        
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, new_client) != 0) {
            perror("Ошибка создания потока");
            close(client_socket);
            free(new_client);
        }
        
        pthread_detach(thread);
    }
    
    close(server_socket);
    return 0;
}
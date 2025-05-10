#include "../include/commands.h"
#include "../include/server.h"
#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     
#include <sys/wait.h>   
#ifndef BUFFER_SIZE
#define BUFFER_SIZE 1024
#endif

void handle_command_mode(int client_socket) {
    char buffer[BUFFER_SIZE];
    send_message(client_socket, "Вы вошли в режим выполнения команд. Для выхода введите exit");
    
    while (1) {
        send_message(client_socket, "Введите команду:");
        receive_message(client_socket, buffer, BUFFER_SIZE);
        
        if (strcmp(buffer, "exit") == 0) {
            break;
        }
        
        FILE* fp = popen(buffer, "r");
        if (fp == NULL) {
            send_message(client_socket, "Ошибка выполнения команды");
            continue;
        }
        
        char result[BUFFER_SIZE] = {0};
        while (fgets(result, sizeof(result), fp) != NULL) {
            send_message(client_socket, result);
        }
        
        int status = pclose(fp);
        if (status == -1) {
            send_message(client_socket, "Ошибка закрытия потока команды");
        }
    }
    
    send_message(client_socket, "Вы вышли из режима выполнения команд");
}
#include "../include/file_transfer.h"
#include "../include/server.h"
#include <stdio.h>      // Для sscanf, snprintf
#include <string.h>     // Для strncmp
#include <unistd.h>     // Для read, write, close
#include <dirent.h>     // Для DIR, opendir, readdir, closedir
#include <sys/stat.h>   // Для stat
#include <sys/socket.h> // Для send, recv
#include <fcntl.h>      // Для open, O_RDONLY и др.

#define FILE_DIR "files/"

void handle_file_transfer_mode(int client_socket) {
    char buffer[BUFFER_SIZE];
    send_message(client_socket, "Вы вошли в режим передачи файлов. Доступные команды:\n"
                  "download <filename> - скачать файл\n"
                  "upload <filename> - загрузить файл\n"
                  "list - список файлов\n"
                  "exit - выход");
    
    while (1) {
        receive_message(client_socket, buffer, BUFFER_SIZE);
        
        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }
        else if (strncmp(buffer, "list", 4) == 0) {
            DIR* dir;
            struct dirent* ent;
            
            if ((dir = opendir(FILE_DIR)) != NULL) {
                send_message(client_socket, "Доступные файлы:");
                while ((ent = readdir(dir)) != NULL) {
                    if (ent->d_type == DT_REG) {
                        send_message(client_socket, ent->d_name);
                    }
                }
                closedir(dir);
            } else {
                send_message(client_socket, "Не удалось открыть директорию с файлами");
            }
        }
        else if (strncmp(buffer, "download", 8) == 0) {
            char filename[256];
            sscanf(buffer + 9, "%255s", filename);
            
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s%s", FILE_DIR, filename);
            
            int fd = open(filepath, O_RDONLY);
            if (fd == -1) {
                send_message(client_socket, "Файл не найден");
                continue;
            }
            
            struct stat st;
            fstat(fd, &st);
            off_t file_size = st.st_size;
            
            char size_msg[64];
            snprintf(size_msg, sizeof(size_msg), "FILE_SIZE:%lld", (long long)file_size);
            send_message(client_socket, size_msg);
            
            char file_buffer[BUFFER_SIZE];
            ssize_t bytes_read;
            
            while ((bytes_read = read(fd, file_buffer, BUFFER_SIZE)) > 0) {
                send(client_socket, file_buffer, bytes_read, 0);
            }
            
            close(fd);
        }
        else if (strncmp(buffer, "upload", 6) == 0) {
            char filename[256];
            sscanf(buffer + 7, "%255s", filename);
            
            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s%s", FILE_DIR, filename);
            
            // Получаем размер файла
            receive_message(client_socket, buffer, BUFFER_SIZE);
            long long file_size;
            sscanf(buffer, "FILE_SIZE:%lld", &file_size);
            
            int fd = open(filepath, O_WRONLY | O_CREAT, 0644);
            if (fd == -1) {
                send_message(client_socket, "Ошибка создания файла");
                continue;
            }
            
            char file_buffer[BUFFER_SIZE];
            ssize_t bytes_received;
            long long total_received = 0;
            
            while (total_received < file_size) {
                bytes_received = recv(client_socket, file_buffer, BUFFER_SIZE, 0);
                if (bytes_received <= 0) break;
                
                write(fd, file_buffer, bytes_received);
                total_received += bytes_received;
            }
            
            close(fd);
            send_message(client_socket, "Файл успешно загружен");
        }
        else {
            send_message(client_socket, "Неизвестная команда");
        }
    }
    
    send_message(client_socket, "Вы вышли из режима передачи файлов");
}
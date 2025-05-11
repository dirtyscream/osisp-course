#include "../include/file_transfer.h"
#include "../include/server.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h> // for atoll()
#include <limits.h> // for PATH_MAX

#define FILE_DIR "files/"
#define MAX_FILENAME_LEN 256

// Helper macro for minimum of two values
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Ensure directory exists
static void ensure_files_dir() {
    struct stat st = {0};
    if (stat(FILE_DIR, &st) == -1) {
        mkdir(FILE_DIR, 0755);
    }
}

void handle_file_transfer_mode(int client_socket) {
    ensure_files_dir();
    char buffer[BUFFER_SIZE];
    
    send_message(client_socket, "\n=== File Transfer Mode ===\n");
    send_message(client_socket, "Available commands:\n"
                  "download <filename> - Download a file\n"
                  "upload <filename> - Upload a file\n"
                  "list - List available files\n"
                  "exit - Exit this mode\n");

    while (1) {
        send_message(client_socket, "ftp> ");
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            break; // Client disconnected
        }
        buffer[bytes_received] = '\0';
        buffer[strcspn(buffer, "\r\n")] = '\0'; // Trim newline

        if (strcmp(buffer, "exit") == 0) {
            break;
        }
        else if (strcmp(buffer, "list") == 0) {
            DIR *dir;
            struct dirent *ent;
            
            if ((dir = opendir(FILE_DIR)) != NULL) {
                send_message(client_socket, "Available files:\n");
                int file_count = 0;
                
                while ((ent = readdir(dir)) != NULL) {
                    if (ent->d_type == DT_REG) {
                        char file_info[512];
                        struct stat st;
                        char filepath[512];
                        snprintf(filepath, sizeof(filepath), "%s%s", FILE_DIR, ent->d_name);
                        
                        if (stat(filepath, &st) == 0) {
                            snprintf(file_info, sizeof(file_info), "%-30s %8lld bytes",
                                    ent->d_name, (long long)st.st_size);
                            send_message(client_socket, file_info);
                            file_count++;
                        }
                    }
                }
                
                if (file_count == 0) {
                    send_message(client_socket, "No files available\n");
                }
                closedir(dir);
            } else {
                send_message(client_socket, "Failed to open directory");
            }
        }
        else if (strncmp(buffer, "download ", 9) == 0) {
            char filename[MAX_FILENAME_LEN];
            strncpy(filename, buffer + 9, MAX_FILENAME_LEN - 1);
            filename[MAX_FILENAME_LEN - 1] = '\0';

            char filepath[PATH_MAX];
            snprintf(filepath, sizeof(filepath), "%s%s", FILE_DIR, filename);
            
            // Check if file exists and is regular file
            struct stat st;
            if (stat(filepath, &st) != 0 || !S_ISREG(st.st_mode)) {
                send_message(client_socket, "Error: File not found");
                continue;
            }

            // Open file
            int fd = open(filepath, O_RDONLY);
            if (fd < 0) {
                send_message(client_socket, "Error: Cannot open file");
                continue;
            }

            // Send file size first
            char header[128];
            snprintf(header, sizeof(header), "FILE_START:%s:%lld\n", filename, (long long)st.st_size);
            send_message(client_socket, header);

            // Send file content
            char file_buffer[BUFFER_SIZE];
            ssize_t bytes_read, bytes_sent;
            off_t total_sent = 0;
            
            while ((bytes_read = read(fd, file_buffer, sizeof(file_buffer))) > 0) {
                bytes_sent = send(client_socket, file_buffer, bytes_read, 0);
                if (bytes_sent <= 0) {
                    break; // Connection error
                }
                total_sent += bytes_sent;
            }

            close(fd);
            
            if (total_sent == st.st_size) {
                send_message(client_socket, "FILE_END:Transfer complete\n");
            } else {
                send_message(client_socket, "FILE_ERR:Transfer interrupted\n");
            }
        }
        else if (strncmp(buffer, "upload ", 7) == 0) {
            char filename[256];
            sscanf(buffer + 7, "%255s", filename);
            
            // Логирование 1
            printf("Upload request for: %s\n", filename);
            
            char filepath[PATH_MAX];
            snprintf(filepath, sizeof(filepath), "%s/%s", FILE_DIR, filename);
            
            // Логирование 2
            printf("Full destination path: %s\n", filepath);
            
            int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("open() failed"); // Выведет причину ошибки
                send_message(client_socket, "Upload failed: cannot create file");
                continue;
            }
            
            // Логирование 3
            printf("File opened for writing, fd=%d\n", fd);
            
            char buf[BUFFER_SIZE];
            ssize_t bytes_received;
            off_t total = 0;
            
            while ((bytes_received = recv(client_socket, buf, sizeof(buf), 0)) > 0) {
                ssize_t written = write(fd, buf, bytes_received);
                if (written != bytes_received) {
                    perror("write() failed");
                    break;
                }
                total += written;
                
                // Логирование 4 (прогресс)
                printf("Received %zd bytes (total: %ld)\n", bytes_received, total);
            }
            
            close(fd);
            
            // Финализируем
            printf("Upload completed. Total bytes: %ld\n", total);
            send_message(client_socket, "Upload complete");
            
            // Проверка существования файла
            if (access(filepath, F_OK) == 0) {
                printf("File verification: EXISTS\n");
            } else {
                printf("File verification: MISSING (errno=%d)\n", errno);
            }
        }
        else {
            send_message(client_socket, "Error: Unknown command\n");
        }
    }

    send_message(client_socket, "Exiting file transfer mode\n");
}
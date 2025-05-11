#include "../include/commands.h"
#include "../include/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>

typedef struct {
    char cwd[PATH_MAX];  
} SessionState;

static SessionState session;

static void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void execute_command(int client_socket, const char* command) {
    if (strncmp(command, "cd ", 3) == 0 || strcmp(command, "cd") == 0) {
        char path[PATH_MAX];
        if (strcmp(command, "cd") == 0) {
            strcpy(path, getenv("HOME") ? getenv("HOME") : "/");
        } else {
            strcpy(path, command + 3);
        }
        if (chdir(path) == -1) {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "cd: %s: %s\n", path, strerror(errno));
            send(client_socket, error_msg, strlen(error_msg), 0);
        }
        getcwd(session.cwd, sizeof(session.cwd));
        return;
    }
    int pipe_stdout[2], pipe_stderr[2];
    pid_t pid;
    if (pipe(pipe_stdout) < 0 || pipe(pipe_stderr) < 0) {
        send_message(client_socket, "Error creating pipe\n");
        return;
    }
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        return;
    }
    pid = fork();
    if (pid < 0) {
        send_message(client_socket, "Error fork\n");
        return;
    }
    if (pid == 0) {  
        
        dup2(pipe_stdout[1], STDOUT_FILENO);
        dup2(pipe_stderr[1], STDERR_FILENO);
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);   
        chdir(session.cwd);    
        char* args[64];
        int i = 0;
        char* token = strtok((char*)command, " ");
        while (token != NULL && i < 63) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL; 
        execvp(args[0], args);
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error command: %s\n", strerror(errno));
        write(STDERR_FILENO, error_msg, strlen(error_msg));
        exit(EXIT_FAILURE);
    } else {  
        
        close(pipe_stdout[1]);
        close(pipe_stderr[1]);
        char buffer[BUFFER_SIZE];
        ssize_t n;
        
        while ((n = read(pipe_stdout[0], buffer, sizeof(buffer))) > 0) {
            send(client_socket, buffer, n, 0);
        }

        while ((n = read(pipe_stderr[0], buffer, sizeof(buffer))) > 0) {
            send(client_socket, buffer, n, 0);
        }
     
        close(pipe_stdout[0]);
        close(pipe_stderr[0]);

        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            char exit_msg[64];
            snprintf(exit_msg, sizeof(exit_msg), "Command ended with code %d\n", WEXITSTATUS(status));
            send_message(client_socket, exit_msg);
        }
    }
}


void handle_command_mode(int client_socket) {
    char buffer[BUFFER_SIZE];
    getcwd(session.cwd, sizeof(session.cwd));  

    send_message(client_socket, "=== Command Mode (fork/exec implementation) ===\n");
    send_message(client_socket, "Type 'exit' to quit\n");

    while (1) { 
        char prompt[PATH_MAX + 64];
        snprintf(prompt, sizeof(prompt), "user@server:%s$ ", session.cwd);
        send_message(client_socket, prompt);
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            break;  
        }
        buffer[bytes_received] = '\0';
        buffer[strcspn(buffer, "\n\r")] = '\0';  
        if (strcmp(buffer, "exit") == 0) {
            send_message(client_socket, "Exiting command mode...\n");
            break;
        }
        if (strlen(buffer) == 0) {
            continue;
        }
        execute_command(client_socket, buffer);
    }
}
/* version 2
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LENGTH 1024
#define MAX_ARGS 512

void execute_command(char *cmd);
void parse_and_execute(char *cmd);

int main() {
    char cmd[MAX_CMD_LENGTH];

    while (1) {
        printf("PUCITshell@%s:- ", getcwd(NULL, 0));
        if (fgets(cmd, MAX_CMD_LENGTH, stdin) == NULL) {
            break; // Exit on Ctrl+D
        }

        // Remove trailing newline
        cmd[strcspn(cmd, "\n")] = 0;

        if (strlen(cmd) == 0) continue; // Skip empty commands
        
        // Execute the command
        execute_command(cmd);
    }
    
    return 0;
}
void execute_command(char *cmd) {
    char *cmd_parts[MAX_ARGS];
    int cmd_count = 0;

    // Tokenize commands separated by pipes '|'
    cmd_parts[cmd_count] = strtok(cmd, "|");
    while (cmd_parts[cmd_count] != NULL) {
        cmd_count++;
        cmd_parts[cmd_count] = strtok(NULL, "|");
    }

    int i, in_fd = 0;

    for (i = 0; i < cmd_count; i++) {
        int pipe_fd[2];
        pipe(pipe_fd);

        if (fork() == 0) { // Child process
            dup2(in_fd, STDIN_FILENO);  // redirect input from previous command
            if (i < cmd_count - 1) {
                dup2(pipe_fd[1], STDOUT_FILENO);
            }
            
            close(pipe_fd[0]);
            parse_and_execute(cmd_parts[i]);
            exit(0);
        } else { // Parent process
            wait(NULL);
            close(pipe_fd[1]);
            in_fd = pipe_fd[0];
        }
    }
}
void parse_and_execute(char *cmd) {
    char *args[MAX_ARGS];
    char *input_file = NULL, *output_file = NULL;
    int i = 0, j = 0;

    // Tokenize and detect redirection
    char *token = strtok(cmd, " ");
    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) input_file = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) output_file = token;
        } else {
            args[j++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[j] = NULL;
    // Handle input redirection
    if (input_file != NULL) {
        int in_fd = open(input_file, O_RDONLY);
        if (in_fd < 0) {
            perror("Input file error");
            exit(1);
        }
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }
    
    
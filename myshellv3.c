/* Version 3
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_CMD_LENGTH 1024
#define MAX_ARGS 512

void execute_command(char *cmd);
void parse_and_execute(char *cmd);
void sigchld_handler(int signo);

int main() {
    signal(SIGCHLD, sigchld_handler);  // Signal handler for background processes

    char cmd[MAX_CMD_LENGTH];

    while (1) {
        printf("PUCITshell@%s:- ", getcwd(NULL, 0));
        if (fgets(cmd, MAX_CMD_LENGTH, stdin) == NULL) {
            break;  // Exit on Ctrl+D
        }

        // Remove trailing newline
        cmd[strcspn(cmd, "\n")] = 0;

        if (strlen(cmd) == 0) continue;  // Skip empty commands

        execute_command(cmd);  // Execute command
    }

    return 0;
}

void execute_command(char *cmd) {
    int background = 0;

    // Check if the command should run in the background
    if (cmd[strlen(cmd) - 1] == '&') {
        background = 1;
        cmd[strlen(cmd) - 1] = '\0';  // Remove '&' from command string
    }

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

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {  // Child process
            dup2(in_fd, STDIN_FILENO);  // Redirect input from previous command
            if (i < cmd_count - 1) {
                dup2(pipe_fd[1], STDOUT_FILENO);  // Redirect output to pipe
            }
            close(pipe_fd[0]);  // Close unused read end
            parse_and_execute(cmd_parts[i]);
            exit(0);
        } else {  // Parent process
            if (!background) {
                waitpid(pid, NULL, 0);  // Wait for foreground process
            } else {
                printf("[1] %d\n", pid);  // Print background process ID
            }
            close(pipe_fd[1]);
            in_fd = pipe_fd[0];
        }
    }
}

void parse_and_execute(char *cmd) {
    char *args[MAX_ARGS];
    char *input_file = NULL, *output_file = NULL;
    int j = 0;

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

    // Handle output redirection
    if (output_file != NULL) {
        int out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) {
            perror("Output file error");
            exit(1);
        }
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }

    // Execute the command
    if (execvp(args[0], args) < 0) {
        perror("Exec failed");
        exit(1);
    }
}

void sigchld_handler(int signo) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
}

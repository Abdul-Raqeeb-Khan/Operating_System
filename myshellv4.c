/*Version 4
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_CMD_LENGTH 1024
#define MAX_ARGS 512
#define HISTORY_SIZE 10

char *history[HISTORY_SIZE];  // Array to hold command history
int history_count = 0;         // Current count of commands in history

void execute_command(char *cmd);
void parse_and_execute(char *cmd);
void add_to_history(char *cmd);
void print_history();
void execute_from_history(int index);

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

        // Check for history command
        if (strcmp(cmd, "history") == 0) {
            print_history();
            continue; // Skip adding this command to history
        }

        // Add to history and check for history commands
        add_to_history(cmd);

        if (cmd[0] == '!') {
            if (isdigit(cmd[1])) {
                int index = atoi(&cmd[1]);
                if (index > 0 && index <= history_count) {
                    execute_from_history(index - 1);
                } else {
                    printf("No such command in history\n");
                }
            } else if (strcmp(cmd, "!-1") == 0) {
                execute_from_history(history_count - 1);
            } else {
                printf("Invalid command\n");
            }
        } else {
            execute_command(cmd); // Execute the command
        }
    }

    // Free command history
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
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
            wait(NULL);
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

void add_to_history(char *cmd) {
    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(cmd);
        history_count++;
    } else {
        free(history[0]); // Free the oldest command
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i]; // Shift commands up
        }
        history[HISTORY_SIZE - 1] = strdup(cmd); // Add new command
    }
}

void print_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

void execute_from_history(int index) {
    if (index >= 0 && index < history_count) {
        printf("Executing: %s\n", history[index]);
        execute_command(history[index]);
    }
}

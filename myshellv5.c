/*Version 5
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_INPUT 1024
#define DELIM " \t\r\n\a"

void start_shell();
char *read_input();
char **parse_input(char *line);
void execute_command(char **args, int background);
int run_builtin_command(char **args);

int main() {
    start_shell();
    return 0;
}

void start_shell() {
    char *input;
    char **args;
    int background;

    do {
        printf("BilalShell:- ");
        input = read_input();
        args = parse_input(input);
        background = 0;

        // Check for background process (&)
        int i = 0;
        while (args[i] != NULL) {
            if (strcmp(args[i], "&") == 0) {
                background = 1;
                args[i] = NULL;
                break;
            }
            i++;
        }

        // Check if it's a built-in command
        if (run_builtin_command(args) == 0) {
            free(input);
            free(args);
            continue;
        }

        if (args[0] != NULL) {
            execute_command(args, background);
        }

        free(input);
        free(args);
    } while (1);
}

char *read_input() {
    char *line = NULL;
    size_t bufsize = 0;
    getline(&line, &bufsize, stdin);
    return line;
}

char **parse_input(char *line) {
    int bufsize = 64, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    token = strtok(line, DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char *));
        }
        token = strtok(NULL, DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void execute_command(char **args, int background) {
    pid_t pid = fork();

    if (pid == 0) {  // Child process
        if (execvp(args[0], args) == -1) {
            perror("BilalShell");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("BilalShell");
    } else {
        if (!background) {
            waitpid(pid, NULL, 0);  // Wait for foreground process
        } else {
            printf("[Running in background] PID: %d\n", pid);
        }
    }
}

int run_builtin_command(char **args) {
    if (args[0] == NULL) {
        return 1;  // Empty command
    }

    // Built-in command: cd
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "BilalShell: expected argument to \"cd\"\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("BilalShell");
            }
        }
        return 0;
    }

    // Built-in command: exit
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    // Built-in command: help
    if (strcmp(args[0], "help") == 0) {
        printf("BilalShell: Supported built-in commands are:\n");
        printf("cd [directory] - Change directory\n");
        printf("exit - Exit the shell\n");
        printf("help - Show this help message\n");
        return 0;
    }

    return 1;  // Not a built-in command
}


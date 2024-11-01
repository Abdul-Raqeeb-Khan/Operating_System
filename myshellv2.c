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
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

void execute_command(char *command) {
    char *arguments[64];
    int i = 0;

    // Tokenize the command string based on spaces
    char *token = strtok(command, " ");
    while (token != NULL) {
        arguments[i++] = token;
        token = strtok(NULL, " ");
    }
    arguments[i] = NULL;

    // Execute the command
    execvp(arguments[0], arguments);

    // If execvp returns, it means an error occurred
    perror("execvp");
    exit(EXIT_FAILURE);
}

void run_commands(char *commands) {
    char *token;
    char *rest = commands;

    // Tokenize the command string based on "||"
    while ((token = strtok_r(rest, "||", &rest))) {
        // Fork a child process
        pid_t pid = fork();

        if (pid == -1) {
            // Fork failed
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process
            execute_command(token);
        } else {
            // Parent process
            int status;
            waitpid(pid, &status, 0);

            // Check if the command was successful
            if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
                // If successful, break out of the loop
                break;
            }
        }
    }
}

void print_subcommands(char *command) {
    char *token;
    char *rest = command;

    // Tokenize the command string based on "&&"
    while ((token = strtok_r(rest, "&&", &rest))) {
        // Print the command
        printf("Command 1: %s\n", token);

        // Check if there's a subsequent command
        if (rest != NULL && strlen(rest) > 0) {
            // Find the position of the next "&&" or end of string
            char *next_and = strstr(rest, "&&");
            if (next_and == NULL) {
                // If there's no "&&" after this command, print the remaining command
                printf("Command 2: %s\n", rest);
                // No need to continue further
                break;
            } else {
                // Print the remaining command until the next "&&"
                int length = next_and - rest;
                char next_command[length + 1];
                strncpy(next_command, rest, length);
                next_command[length] = '\0';

                // Remove leading '&' if present
                if (next_command[0] == '&') {
                    printf("Command 3: %s\n", next_command + 1);

                } else {
                    printf("Command 4: %s\n", next_command);
                }

                // Move the rest pointer to the beginning of the next command
                rest = next_and + 2; // Skip "&&"
            }
        }
    }
}

int main() {
    // Example command containing || and &&
    char command[] = "date || ls && ls || ps && ls . || pwd && ls && ls";

    // Print each subcommand
    print_subcommands(command);

    return 0;
}

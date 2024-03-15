#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_ARGUMENTS 10

void execute_command(char *command) {
    char *args[MAX_ARGUMENTS];
    int arg_count = 0;

    // Tokenize the command into arguments
    char *token = strtok(command, " ");
    while (token != NULL && arg_count < MAX_ARGUMENTS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;

    // Fork to create a child process
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process executes the command
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process waits for the child to complete
        wait(NULL);
    }
}

int main(int argc, char *argv[]) {
    char command[MAX_COMMAND_LENGTH];

    int print_prompt = 1; // Flag to control prompt printing

    if (argc > 1 && strcmp(argv[1], "newt") == 0) {
        // This is a new shell, no prompt needed
        print_prompt = 0;
    } else {
        print_prompt = 1;
    }

    while (1) {
        if (print_prompt) {
            printf("shell24$ ");
            fflush(stdout);
        }

        // Read user input
        if (fgets(command, sizeof(command), stdin) == NULL) {
            fprintf(stderr, "Error reading command\n");
            continue;
        }

        // Remove trailing newline character
        command[strcspn(command, "\n")] = '\0';

        // Check for newt command to create a new shell24
        if (strcmp(command, "newt") == 0) {
            // Fork to create a new shell24
            pid_t pid = fork();
            if (pid < 0) {
                fprintf(stderr, "Fork failed\n");
                continue;
            } else if (pid == 0) {
                // Child process starts a new shell24
                execlp("./shell24", "./shell24", "newt", NULL);
                exit(EXIT_SUCCESS); // Exit successfully to prevent further execution in child
            }
        } else {
            // Execute the entered command
            execute_command(command);
        }

        print_prompt = 1; // Set the flag to print prompt after executing command
    }

    return 0;
}

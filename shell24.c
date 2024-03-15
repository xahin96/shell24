#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 100

void execute_command(char *command) {
    // Fork to create a child process
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork failed\n");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process executes the command
        char *args[] = {command, NULL};
        if (execvp(command, args) == -1) {
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
            if (fork() == 0) {
                // Child process starts a new shell24
                execlp("./shell24", "./shell24", "newt", NULL);
                exit(EXIT_SUCCESS); // Exit successfully to prevent further execution in child
            }
        } else {
            // Execute the entered command
            execute_command(command);
        }
        // Reset the prompt printing flag for the next iteration
        print_prompt = 1;
    }
    return 0;
}

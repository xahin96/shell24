#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGUMENTS 6 // Including the command itself

// Function to read user input
char *read_input() {
    char *input = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    if (input == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    printf("shell24$ ");
    fgets(input, MAX_COMMAND_LENGTH, stdin);
    return input;
}

// Function to parse input into command and arguments
void parse_input(char *input, char *command, char *arguments[MAX_ARGUMENTS]) {
    char *token;
    int i = 0;

    // Split input by spaces and newline
    token = strtok(input, " \n");
    while (token != NULL) {
        if (i == 0) {
            strcpy(command, token);
        } else {
            arguments[i - 1] = strdup(token);
        }
        token = strtok(NULL, " \n");
        i++;
    }
    arguments[i - 1] = NULL; // Null-terminate the argument list
}

// Function to validate argc
int is_argc_valid(int argc) {
    return (argc >= 1 && argc <= 5);
}

// Function to execute command
void execute_command(char *command, char *arguments[MAX_ARGUMENTS]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        if (execvp(command, arguments) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        wait(NULL);
    }
}

int main() {
    char *input;
    char command[MAX_COMMAND_LENGTH];
    char *arguments[MAX_ARGUMENTS];

    while (1) {
        input = read_input();
        parse_input(input, command, arguments);
        if (!is_argc_valid(strlen(command) + 1)) {
            printf("Error: Number of arguments out of range (1-5).\n");
            continue;
        }
        execute_command(command, arguments);
        free(input);
    }

    return 0;
}

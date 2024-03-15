#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 100

int has_pipe(char *command) {
    char *found_pipe = strstr(command, "|");
    char *found_or = strstr(command, "||");
    return found_pipe != NULL && (found_or == NULL || found_pipe < found_or);
}

int has_or_operator(char *command) {
    char *token = strtok(command, " ");
    while (token != NULL) {
        if (strcmp(token, "||") == 0) {
            return 1;
        }
        token = strtok(NULL, " ");
    }
    return 0;
}

int has_output_redirect(char *command) {
    char *found = strstr(command, ">");
    return found != NULL && (found[1] != '>' || strlen(found) == 1);
}

int has_append_redirect(char *command) {
    return strstr(command, ">>") != NULL;
}

int has_hash(char *command) {
    return strchr(command, '#') != NULL;
}

int has_input_redirect(char *command) {
    return strchr(command, '<') != NULL;
}

int has_and_operator(char *command) {
    char *found = strstr(command, "&&");
    return found != NULL && (found[2] == '\0' || found[2] == ' ');
}

int has_background_process(char *command) {
    char *found = strstr(command, "&");
    return found != NULL && (strcmp(found, "&&") != 0 || strlen(found) == 1);
}

int has_sequential_execution(char *command) {
    return strchr(command, ';') != NULL;
}

void execute_command(char *command) {
    // Placeholder for actual execution logic
    printf("Executing command: %s\n", command);
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

        printf("command: %s\n", command);

        // Check for newt command to create a new shell24
        if (strcmp(command, "newt") == 0) {
            // Fork to create a new shell24
            if (fork() == 0) {
                // Child process starts a new shell24
                execlp("./shell24", "./shell24", "newt", NULL);
                exit(EXIT_SUCCESS); // Exit successfully to prevent further execution in child
            }
        }
        else {
            // Search for special characters
            if (has_hash(command)) {
                printf("Hash found in command: %s\n", command);
            } else if (has_pipe(command)) {
                printf("Pipe found in command: %s\n", command);
            } else if (has_output_redirect(command)) {
                printf("Output redirection found in command: %s\n", command);
            } else if (has_append_redirect(command)) {
                printf("Append redirection found in command: %s\n", command);
            } else if (has_input_redirect(command)) {
                printf("Input redirection found in command: %s\n", command);
            } else if (has_and_operator(command)) {
                printf("AND operator found in command: %s\n", command);
            } else if (has_or_operator(command)) {
                printf("OR operator found in command: %s\n", command);
            } else if (has_background_process(command)) {
                printf("Background process found in command: %s\n", command);
            } else if (has_sequential_execution(command)) {
                printf("Sequential execution found in command: %s\n", command);
            } else {
                // If no special characters found, print the command itself
                printf("Plain command: %s\n", command);
            }
        }
        // Reset the prompt printing flag for the next iteration
        print_prompt = 1;
    }
    return 0;
}


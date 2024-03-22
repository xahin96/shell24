#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void replaceCharacter(char *str, char oldChar, char newChar) {
    int i;
    int length = strlen(str);

    for (i = 0; i < length; i++) {
        if (str[i] == oldChar) {
            str[i] = newChar;
        }
    }
}

int execute_command_sequence_status(const char *full_command) {
    // Allocate memory for a copy of the full command
    char command[strlen(full_command) + 1];
    strcpy(command, full_command);

    // Allocate memory for arguments array
    char *args[strlen(full_command) / 2 + 1]; // Rough estimate of maximum number of arguments
    int argc = 0;

    // Tokenize the command based on spaces, preserving quoted arguments
    char *token = strtok(command, " ");
    while (token != NULL) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;  // Terminate the array with NULL

    // Fork a new process
    pid_t pid = fork();

    if (pid == -1) {
        // Fork failed
        perror("fork");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        // Child process: execute the command
        execvp(args[0], args);

        // If execvp fails, print an error message and return failure status
        perror("execvp");
        return EXIT_FAILURE;
    } else {
        // Parent process: wait for the child to finish
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            // Child process terminated normally
            return WEXITSTATUS(status);
        } else {
            // Child process terminated abnormally
            return EXIT_FAILURE;
        }
    }
}

char** get_subcommands_by_or(char *command) {
    // Allocate memory for an array of strings to hold subcommands
    char** subcommands_or = malloc(sizeof(char*) * 100); // Assuming a maximum of 100 subcommands
    if (subcommands_or == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    char *start = command;
    char *end = command;
    int length = strlen(command);
    int i;

    for (i = 0; i < length; i++) {
        if (command[i] == '|' && command[i + 1] == '|') {
            // Found "||", extract the subcommand
            int sub_len = end - start;
            subcommands_or[count] = malloc(sub_len + 1);
            if (subcommands_or[count] == NULL) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }
            strncpy(subcommands_or[count], start, sub_len);
            subcommands_or[count][sub_len] = '\0';
            replaceCharacter(subcommands_or[count], '&', ' '); // Replace '&' with ' '
            count++;

            // Move start pointer to the beginning of the next command
            start = &command[i + 2];
            end = start;
        } else {
            end++;
        }
    }

    // Extract the last subcommand
    int sub_len = end - start;
    subcommands_or[count] = malloc(sub_len + 1);
    if (subcommands_or[count] == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    strncpy(subcommands_or[count], start, sub_len);
    subcommands_or[count][sub_len] = '\0';
    replaceCharacter(subcommands_or[count], '&', ' '); // Replace '&' with ' '
    count++;

    // Null-terminate the array
    subcommands_or[count] = NULL;

    return subcommands_or;
}

char** get_subcommands(char *command) {
    // Allocate memory for an array of strings to hold subcommands
    char** subcommands = malloc(sizeof(char*) * 100); // Assuming a maximum of 100 subcommands
    if (subcommands == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    char *start = command;
    char *end = command;
    int length = strlen(command);
    int i;

    for (i = 0; i < length; i++) {
        if (command[i] == '&' && command[i + 1] == '&') {
            // Found "&&", extract the subcommand
            int sub_len = end - start;
            subcommands[count] = malloc(sub_len + 1);
            if (subcommands[count] == NULL) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }
            strncpy(subcommands[count], start, sub_len);
            subcommands[count][sub_len] = '\0';
            count++;

            // Move start pointer to the beginning of the next command
            start = &command[i + 2];
            end = start;
        } else {
            end++;
        }
    }

    // Extract the last subcommand
    int sub_len = end - start;
    subcommands[count] = malloc(sub_len + 1);
    if (subcommands[count] == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    strncpy(subcommands[count], start, sub_len);
    subcommands[count][sub_len] = '\0';
    count++;

    // Null-terminate the array
    subcommands[count] = NULL;

    return subcommands;
}

int main() {
    // Example command containing || and &&
    char command[] = "datee || pwd || ls && date || pwd || date && date || pwd && pwd && datee";

    // Get the array of subcommands
    char** subcommands = get_subcommands(command);

    // Print each subcommand from the array
    for (int i = 0; subcommands[i] != NULL; i++) {
        printf("Subcommand_or %d: %s\n", i + 1, subcommands[i]);
        char **subcommands_or = get_subcommands_by_or(subcommands[i]); // Change this line
        for (int j = 0; subcommands_or[j] != NULL; j++) {
            replaceCharacter(subcommands_or[j], '|', ' '); // Replace '|' with ' '
            replaceCharacter(subcommands_or[j], '&', ' '); // Replace '|' with ' '
//            printf("Subcommand_or %d: %s\n", j + 1, subcommands_or[j]);
            int status = execute_command_sequence_status(subcommands_or[j]);
            if (status == EXIT_SUCCESS) {
                break;
            }
        }
        // Free memory allocated for subcommands_or
        for (int k = 0; subcommands_or[k] != NULL; k++) {
            free(subcommands_or[k]);
        }
        free(subcommands_or);
    }

    // Free memory allocated for subcommands
    for (int i = 0; subcommands[i] != NULL; i++) {
        free(subcommands[i]);
    }
    free(subcommands);

    return 0;
}

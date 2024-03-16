#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> // Include fcntl.h for open function


#define MAX_TOTAL_LENGTH 10000 // Adjust this according to your needs
#define MAX_COMMAND_LENGTH 100
#define MAX_FILENAME_LENGTH 1000

int special_space_count = 0;
int special_and_or_count = 0;
int special_character_count = 0;

// Function for concatenating files and printing the content
// receives the list of file names a parameter
void concatenate_files(const char **fileNames) {
    char buffer[MAX_TOTAL_LENGTH]; // Buffer to store concatenated content
    buffer[0] = '\0'; // Initialize buffer as an empty string

    // Loop through each file name
    for (int i = 0; i < special_character_count; i++) {
        // Open file for reading
        FILE *file = fopen(fileNames[i], "r");
        if (file == NULL) {
            fprintf(stderr, "Failed to open file: %s\n", fileNames[i]);
            continue; // Skip to next file if opening fails
        }
        // Read content of the file and append it to the buffer
        char line[MAX_FILENAME_LENGTH];
        while (fgets(line, MAX_FILENAME_LENGTH, file) != NULL) {
            strcat(buffer, line);
        }
        // Close the file
        fclose(file);
    }

    // Print the concatenated content
    printf("%s\n", buffer);
}

// Function for executing commands that contains >, >> & <
void execute_command_file(char *command) {
    system(command);
}

// Function to split command based on OR operator
char **split_by_operator(char *command, char *special_character) {
    special_character_count = 0;
    char *token;
    char **commands = malloc(MAX_COMMAND_LENGTH * sizeof(char *));

    // Split command by "||" and store each command in the array
    token = strtok(command, special_character);
    while (token != NULL) {
        commands[special_character_count++] = token;
        token = strtok(NULL, special_character);
    }
    commands[special_character_count] = NULL; // Null-terminate the array
    return commands;
}
char **split_by_space_operator(char *command, char *special_character) {
    special_space_count = 0;
    char *token;
    char **commands = malloc(MAX_COMMAND_LENGTH * sizeof(char *));

    // Split command by "||" and store each command in the array
    token = strtok(command, special_character);
    while (token != NULL) {
        commands[special_space_count++] = token;
        token = strtok(NULL, special_character);
    }
    commands[special_space_count] = NULL; // Null-terminate the array
    return commands;
}
char **split_by_and_or_operator(char *command, char *special_character1, char *special_character2) {
    special_and_or_count = 0;
    char *token;
    char **commands = malloc(MAX_COMMAND_LENGTH * sizeof(char *));

    // Split command by special_character1 and store each token in the array
    token = strtok(command, special_character1);
    while (token != NULL) {
        // Split each token by special_character2
        char *sub_token = strtok(token, special_character2);
        while (sub_token != NULL) {
            commands[special_and_or_count++] = sub_token;
            sub_token = strtok(NULL, special_character2);
        }
        token = strtok(NULL, special_character1);
    }
    commands[special_and_or_count] = NULL; // Null-terminate the array
    return commands;
}

int has_pipe(char *command) {
    char *found_pipe = strstr(command, "|");
    char *found_or = strstr(command, "||");
    return found_pipe != NULL && (found_or == NULL || found_pipe < found_or);
}

int has_or_operator(char *command) {
    char *command_copy = strdup(command); // Make a copy of the command
    char *token = strtok(command_copy, " ");
    while (token != NULL) {
        if (strcmp(token, "||") == 0) {
            free(command_copy); // Free the memory allocated for the copy
            return 1;
        }
        token = strtok(NULL, " ");
    }
    free(command_copy); // Free the memory allocated for the copy
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
    int length = strlen(command);
    return length > 0 && command[length - 1] == '&';
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
            printf("\nshell24$ ");
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
            // Search for special characters
            if (has_hash(command)) {
                //a.txt#b.txt#c.txt#d.txt#e.txt#f.txt#g.txt
                // Split command by OR operator
                char **or_commands = split_by_operator(command, "#");
                if (special_character_count > 5) {
                    printf("Maximum 5 # can be handled at a time\n");
                } else {
                    concatenate_files(or_commands);
                }
                // Free memory allocated for command array
                free(or_commands);
            }
            else if (has_pipe(command)) {
                printf("Pipe found in command: %s\n", command);
            }
            else if (has_output_redirect(command)) {
                printf("Output redirection found in command: %s\n", command);
                execute_command_file(command);
            }
            else if (has_append_redirect(command)) {
                printf("Append redirection found in command: %s\n", command);
                execute_command_file(command);
            }
            else if (has_input_redirect(command)) {
                printf("Input redirection found in command: %s\n", command);
                execute_command_file(command);
            }
            else if (has_and_operator(command)) {
                printf("AND operator found in command: %s\n", command);
            }
            else if (has_or_operator(command)) {
                printf("OR operator found in command: %s\n", command);
                char **and_or_commands = split_by_and_or_operator(command, "&&", "||");

                if (special_and_or_count > 5) {
                    printf("Maximum 5 ; can be handled at a time\n");
                } else {
                    for (int i = 0; i < special_and_or_count; i++) {
                        printf("%d - %s\n", i, and_or_commands[i]);
                    }
                }

                free(and_or_commands);
            }
            else if (has_background_process(command)) {
                printf("Background process found in command: %s\n", command);
            }
            else if (has_sequential_execution(command)) {
                char **sequential_commands = split_by_operator(command, ";");
                if (special_character_count > 5) {
                    printf("Maximum 5 ; can be handled at a time\n");
                } else {
                    for (int i = 0; i < special_character_count; i++) {
                        char **specific_command = split_by_space_operator(sequential_commands[i], " ");
                        if (special_space_count > 6) {
                            printf("Maximum 5 args can be handled at a time\n");
                            break;
                        } else {
                            execute_command_file(sequential_commands[i]);
                        }
                        free(specific_command);
                    }
                }
                // Free memory allocated for command array
                free(sequential_commands);
            }
            else {
                // If no special characters found, print the command itself
                printf("Plain command: %s\n", command);
            }
        }
        // Reset the prompt printing flag for the next iteration
        print_prompt = 1;
    }
    return 0;
}

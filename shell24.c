#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> // Include fcntl.h for open function
#include <signal.h>


#define MAX_TOTAL_LENGTH 10000 // Adjust this according to your needs
#define MAX_COMMAND_LENGTH 100
#define MAX_FILENAME_LENGTH 1000

pid_t background_pid;
int special_space_count = 0;
int special_character_count = 0;


void execute_command_in_background(char *command) {
    // Fork to create a new process
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        // Detach from the controlling terminal
        if (setsid() == -1) {
            perror("setsid");
            exit(EXIT_FAILURE);
        }

        // Execute the command in the background
        if (system(command) == -1) {
            perror("system");
            exit(EXIT_FAILURE);
        }
        // This part of code will be reached only if system call fails
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        // Store the PID of the background process
        background_pid = pid;
        // Print background process info
        printf("Background process started with PID: %d\n", pid);
        // Wait for the child process to terminate to prevent zombie processes
        waitpid(pid, NULL, WNOHANG);
    }
}

void bring_background_process_to_foreground() {
    if (background_pid == -1) {
        printf("No background process to bring to foreground\n");
        return;
    }

    // Send SIGCONT signal to the background process to bring it to the foreground
    if (kill(background_pid, SIGCONT) == -1) {
        perror("kill");
        return;
    }

    // Wait for the background process to finish and reclaim the terminal
    waitpid(background_pid, NULL, 0);

    // Reset the background PID
    background_pid = -1;
}

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
//    system(command);
    char *args[MAX_COMMAND_LENGTH]; // Array to store command and its arguments
    int i = 0;

    // Tokenize the command string
    char *token = strtok(command, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL; // Null-terminate the argument list

    // Execute the command using execvp
    if (execvp(args[0], args) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}

char **split_by_operator(const char *command, const char *special_character) {
    special_character_count = 0;
    char *token;
    char *command_copy = strdup(command); // Create a copy of the command
    char **commands = malloc(MAX_COMMAND_LENGTH * sizeof(char *));

    // Split command by the special character and store each command in the array
    token = strtok(command_copy, special_character);
    while (token != NULL) {
        commands[special_character_count++] = strdup(token); // Create a copy of each token
        token = strtok(NULL, special_character);
    }
    commands[special_character_count] = NULL; // Null-terminate the array

    free(command_copy); // Free the copy of the command
    return commands;
}
char **split_by_space_operator(const char *command, const char *special_character) {
    special_space_count = 0;
    char *token;
    char *command_copy = strdup(command); // Create a copy of the command
    char **commands = malloc(MAX_COMMAND_LENGTH * sizeof(char *));

    // Split command by "||" and store each command in the array
    token = strtok(command_copy, special_character);
    while (token != NULL) {
        commands[special_space_count++] = strdup(token); // Create a copy of each token
        token = strtok(NULL, special_character);
    }
    commands[special_space_count] = NULL; // Null-terminate the array

    free(command_copy); // Free the copy of the command
    return commands;
}

// Function to count the total number of occurrences of "||" and "&&" in a string
int count_operators(char *str) {
    int count = 0;
    char *ptr = str;
    while ((ptr = strstr(ptr, "||")) != NULL) {
        count++;
        ptr += 2; // Move the pointer to the next character after "||"
    }
    ptr = str;
    while ((ptr = strstr(ptr, "&&")) != NULL) {
        count++;
        ptr += 2; // Move the pointer to the next character after "&&"
    }
    return count;
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

void execute_command_sequence(const char *full_command) {
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
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process: execute the command
        execvp(args[0], args);

        // If execvp fails, print an error message
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process: wait for the child to finish
        int status;
        waitpid(pid, &status, 0);
    }
}

void execute_output_redirection_command(const char *full_command, const char *output_file) {
    // Fork a new process
    pid_t pid = fork();

    if (pid == -1) {
        // Fork failed
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process

        // Open the output file for writing
        printf("output_file: %s\n", output_file);
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // Redirect stdout to the output file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // Close the file descriptor
        close(fd);

        // Execute the command
        execlp("sh", "sh", "-c", full_command, NULL);

        // If execlp fails, print an error message
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process: wait for the child to finish
        int status;
        waitpid(pid, &status, 0);
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
        }
        else {

            // DONE # Text file (.txt) concatenation
            if (has_hash(command)) {
                //a.txt#b.txt#c.txt#d.txt#e.txt#f.txt#g.txt
                // Split command by OR operator
                char **hash_commands = split_by_operator(command, "#");
                if (special_character_count > 5) {
                    printf("Maximum 5 # can be handled at a time\n");
                } else {
                    concatenate_files(hash_commands);
                }
                // Free memory allocated for command array
                free(hash_commands);
            }

            // | Piping
            else if (has_pipe(command)) {
                printf("Pipe found in command: %s\n", command);
            }

            // DONE > Redirection
            // echo "doo doo" > output.txt
            else if (has_output_redirect(command)) {
                printf("Output redirection found in command: %s\n", command);
                char **output_redirection_commands = split_by_operator(command, ">");
                if (special_character_count > 2) {
                    printf("Maximum 1 > can be handled at a time\n");
                } else {
                    execute_output_redirection_command(
                    output_redirection_commands[0],
                    output_redirection_commands[1]
                    );
                }
                // Free memory allocated for command array
                free(output_redirection_commands);
            }

            // >> Redirection
            else if (has_append_redirect(command)) {
                printf("Append redirection found in command: %s\n", command);
                execute_command_file(command);
            }

            // < Redirection
            else if (has_input_redirect(command)) {
                printf("Input redirection found in command: %s\n", command);
                execute_command_file(command);
            }

            // && Conditional Execution
            else if (has_and_operator(command)) {
                printf("AND operator found in command: %s\n", command);
                int total_operators = count_operators(command);

                if (total_operators > 5) {
                    printf("Maximum 5 || or && can be handled at a time\n");
                } else {
                    execute_command_file(command);
                }
            }

            // || Conditional Execution
            else if (has_or_operator(command)) {
                printf("OR operator found in command: %s\n", command);
                int total_operators = count_operators(command);

                if (total_operators > 5) {
                    printf("Maximum 5 || or && can be handled at a time\n");
                } else {
                    execute_command_file(command);
                }
            }

            // & Background Processing
            else if (has_background_process(command)) {
                printf("Background process found in command: %s\n", command);
                command[strlen(command) - 1] = '\0'; // Remove the '&' character
                execute_command_in_background(command);
            }
            else if (strcmp(command, "fg") == 0) {
                // Bring the last background process to the foreground
                bring_background_process_to_foreground();
            }

            // DONE ; Sequential execution
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
                            execute_command_sequence(sequential_commands[i]);
                        }
                        free(specific_command);
                    }
                }
                // Free memory allocated for command array
                free(sequential_commands);
            }

            // DONE Plain command
            else {
                // If no special characters found, print the command itself
                printf("Plain command: %s\n", command);
                execute_command_sequence(command);
            }
        }
        // Reset the prompt printing flag for the next iteration
        print_prompt = 1;
    }
    return 0;
}

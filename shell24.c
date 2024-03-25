#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_PIPES 6
#define MAX_COMMAND_LENGTH 100

pid_t background_pid;
int special_space_count = 0;
int special_character_count = 0;

// Run a process in the run_in_background
void run_in_background (char *command) {
    printf("The program input by user will be run in the run_in_background\n");
    // int argc;
    // split_each_command(command, &argc);
    printf("run_in_background process is %s\n", command);

    int pid = fork();
    // child
    if ( pid == 0 ) {
        printf("%d - %d - %d \n", getpid(), getppid(), getpgid(getpid()));
        setsid();
        printf("%d - %d - %d \n", getpid(), getppid(), getpgid(getpid()));
        printf("The child process will be differentiated and run in the run_in_background\n");
        execlp(command, command, NULL);
        printf("Differentiation unsuccessful\n");
    }
        // parent
    else if (pid > 0) {
        background_pid = pid;
        kill(pid, SIGSTOP);
        printf("run_in_background process pid = %d\n", pid);
    } else {
        printf("Fork failed\n");
    }
}

// Bring a process back into the bring_to_foreground
void bring_to_foreground () {
    if (background_pid == -1) {
        printf("There is no run_in_background process\n");
    } else {
        if (fork()==0) {
            kill(background_pid, SIGCONT);
            background_pid = -1;
        } else {
            wait(NULL);
        }
    }
}

void concatenate_files(const char **fileNames) {
    // Fork to create a new process
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        // Build the argument list for execvp
        char *args[special_character_count + 2]; // Add one for the command name and one for NULL terminator
        args[0] = "cat"; // Command name
        for (int i = 0; i < special_character_count; i++) {
            args[i + 1] = strdup(fileNames[i]); // File names
        }
        args[special_character_count + 1] = NULL; // Null-terminate the argument list

        // Execute the cat command
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        // Wait for the child process to finish
        int status;
        waitpid(pid, &status, 0);
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

void execute_output_append_redirection_command(const char *full_command, const char *output_file) {
    // Fork a new process
    pid_t pid = fork();

    if (pid == -1) {
        // Fork failed
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process

        // Open the output file for appending
        int fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
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

void execute_input_redirection_command(const char *full_command, const char *input_file) {
    // Fork a new process
    pid_t pid = fork();

    if (pid == -1) {
        // Fork failed
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process

        // Open the input file for reading
        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // Redirect stdin to the input file
        if (dup2(fd, STDIN_FILENO) == -1) {
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

void execute_piped_commands(char *command) {
    char *commands[MAX_PIPES]; // Array to store individual commands
    int num_pipes = 0; // Count of pipes found

    // Tokenize the command by pipe character
    char *token = strtok(command, "|");
    while (token != NULL && num_pipes < MAX_PIPES) {
        commands[num_pipes++] = token;
        token = strtok(NULL, "|");
    }

    // Set up pipes
    int pipes[num_pipes - 1][2]; // Array to hold pipe file descriptors

    // Create pipes
    for (int i = 0; i < num_pipes - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // Fork processes for each command
    for (int i = 0; i < num_pipes; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process

            // Close unused read ends of pipes
            for (int j = 0; j < num_pipes - 1; j++) {
                if (j != i - 1 && j != i) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }

            // Redirect stdin if not the first command
            if (i != 0) {
                close(STDIN_FILENO);
                dup(pipes[i - 1][0]);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }

            // Redirect stdout if not the last command
            if (i != num_pipes - 1) {
                close(STDOUT_FILENO);
                dup(pipes[i][1]);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            // Execute the command
            execute_command_sequence(commands[i]);
            exit(EXIT_SUCCESS); // Child process exits after command execution
        }
    }

    // Close all pipe descriptors in the parent process
    for (int i = 0; i < num_pipes - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < num_pipes; i++) {
        wait(NULL);
    }
}

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
        exit(EXIT_FAILURE); // Terminate the child process
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

void handle_and_or(char *command){
    // Get the array of subcommands
    char** subcommands = get_subcommands(command);

    // Print each subcommand from the array
    for (int i = 0; subcommands[i] != NULL; i++) {
        char **subcommands_or = get_subcommands_by_or(subcommands[i]);
        for (int j = 0; subcommands_or[j] != NULL; j++) {
            replaceCharacter(subcommands_or[j], '|', ' ');
            replaceCharacter(subcommands_or[j], '&', ' ');
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
                execlp("xterm", "xterm", "-e", "./shell24", "newt", NULL);
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
                execute_piped_commands(command);
            }

            // DONE > Redirection
            else if (has_output_redirect(command)) {
                char **output_redirection_commands = split_by_operator(command, ">");
                if (special_character_count > 2) {
                    printf("Maximum 1 > can be handled at a time\n");
                } else if (special_character_count == 2) {
                    execute_output_redirection_command(
                    output_redirection_commands[0],
                    output_redirection_commands[1]
                    );
                } else {
                    printf("Invalid output redirection command");
                }
                // Free memory allocated for command array
                free(output_redirection_commands);
            }

            // DONE >> Redirection
            else if (has_append_redirect(command)) {
                char **output_append_redirection_commands = split_by_operator(command, ">");
                if (special_character_count > 2) {
                    printf("Maximum 1 >> can be handled at a time\n");
                } else if (special_character_count == 2) {
                    execute_output_append_redirection_command(
                            output_append_redirection_commands[0],
                            output_append_redirection_commands[1]
                    );
                } else {
                    printf("Invalid output append redirection command");
                }
                // Free memory allocated for command array
                free(output_append_redirection_commands);
            }

            // DONE < Redirection
            else if (has_input_redirect(command)) {
                char **input_redirection_commands = split_by_operator(command, "<");
                if (special_character_count > 2) {
                    printf("Maximum 1 < can be handled at a time\n");
                } else if (special_character_count == 2) {
                    execute_input_redirection_command(
                            input_redirection_commands[0],
                            input_redirection_commands[1]
                    );
                } else {
                    printf("Invalid input redirection command");
                }
                // Free memory allocated for command array
                free(input_redirection_commands);
            }

            // DONE && Conditional Execution
            else if (has_and_operator(command)) {
                int total_operators = count_operators(command);

                if (total_operators > 4) {
                    printf("Maximum 5 || or && can be handled at a time\n");
                } else {
                    handle_and_or(command);
                }
            }

            // DONE || Conditional Execution
            else if (has_or_operator(command)) {
                int total_operators = count_operators(command);

                if (total_operators > 4) {
                    printf("Maximum 5 || or && can be handled at a time\n");
                } else {
                    handle_and_or(command);
                }
            }

            // DONE & Background Processing
            else if (has_background_process(command)) {
                char **bg_commands = split_by_operator(command, " ");
                run_in_background(bg_commands[0]);
                free(bg_commands);

            }
            else if (strcmp(command, "fg") == 0) {
                // Bring the last run_in_background process to the bring_to_foreground
                bring_to_foreground();
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_ALLOWED_PIPES 6
#define MAX_COMMAND_LENGTH 100

// Used for storing the PID of the bg process
pid_t background_pid;
// For counting the total number of space
int special_space_count = 0;
// For counting the total number of special character
int special_character_count = 0;


// function for running a program in background
void run_in_background (char *command) {
    // forking to create a child process
    int pid = fork();
    // the child process
    if ( pid == 0 ) {
        // setting a new session for child and setting the PGID to its
        // own PID
        setsid();
        // running the program
        execlp(command, command, NULL);
        perror("execlp");
    }
    // the parent process
    else if (pid > 0) {
        // storing the pid of the child process so that it can be brought
        // back to foreground later
        background_pid = pid;
        // sending SIGSTOP to send the child to background finally
        kill(pid, SIGSTOP);
    } else {
        printf("Fork failed\n");
    }
}


// function for bringing specific program to foreground
void bring_to_foreground () {
    if (background_pid == -1) {
        printf("There is no run_in_background process\n");
    } else {
        // forking and selecting child process to execute command
        if (fork()==0) {
            // sending SIGCONT to bring the program to foreground
            kill(background_pid, SIGCONT);
            // resetting the background_pid for future use
            background_pid = -1;
        } else {
            // waiting from parent process
            wait(NULL);
        }
    }
}


// function for concatenating files in the file list
void concatenate_files(const char **fileNames) {
    // forking to run command from child
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // selecting child process
    if (pid == 0) {
        // declaring a array for soring the commands to run with two extra
        // space. one for cat command and one for null termination
        char *command_array[special_character_count + 2];
        command_array[0] = "cat";
        // generating the command array with all the filenames
        for (int i = 0; i < special_character_count; i++) {
            command_array[i + 1] = strdup(fileNames[i]);
        }
        command_array[special_character_count + 1] = NULL;

        // finally executing the command
        if (execvp(command_array[0], command_array) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    // parent block
    else {
        int status;
        // waiting for child to finish
        waitpid(pid, &status, 0);
    }
}


// Splits a command with a special character and returns the sub command list
char **split_by_operator(const char *command, const char *special_character) {
    // restarting the special character count variable
    special_character_count = 0;
    // variable for storing the temporary sub command
    char *sub_command_string;
    // Copying the command so that the main string doesn't get manipulated
    char *command_copy = strdup(command);
    // array for storing all the sub commands
    char **sub_command_list = malloc(MAX_COMMAND_LENGTH * sizeof(char *));

    // tokenizing the command_copy variable by special_character
    sub_command_string = strtok(command_copy, special_character);
    // looking for all the tokens for storing
    while (sub_command_string != NULL) {
        // inserting sub command to array
        sub_command_list[special_character_count++] = strdup(sub_command_string);
        // continuing the sub command splitting
        sub_command_string = strtok(NULL, special_character);
    }
    // null terminating for ending the array
    sub_command_list[special_character_count] = NULL;
    // freeing the memory
    free(command_copy);
    return sub_command_list;
}


// Splits a command with space and returns the sub command list
char **split_by_space_operator(const char *command, const char *special_character) {
    // restarting the space count variable
    special_space_count = 0;
    // variable for storing the temporary sub command
    char *sub_command_string;
    // Copying the command so that the main string doesn't get manipulated
    char *command_copy = strdup(command);
    // array for storing all the sub commands
    char **commands = malloc(MAX_COMMAND_LENGTH * sizeof(char *));

    // tokenizing the command_copy variable by special_character
    sub_command_string = strtok(command_copy, special_character);
    // looking for all the tokens for storing
    while (sub_command_string != NULL) {
        // inserting sub command to array
        commands[special_space_count++] = strdup(sub_command_string);
        // continuing the sub command splitting
        sub_command_string = strtok(NULL, special_character);
    }
    // null terminating for ending the array
    commands[special_space_count] = NULL;
    // freeing the memory
    free(command_copy);
    return commands;
}


// function for counting the total number of && or || in a command
int count_operators(char *base_command) {
    int and_or_counter = 0;
    char *command_string = base_command;
    while ((command_string = strstr(command_string, "||")) != NULL) {
        and_or_counter++;
        command_string += 2;
    }
    command_string = base_command;
    while ((command_string = strstr(command_string, "&&")) != NULL) {
        and_or_counter++;
        command_string += 2;
    }
    return and_or_counter;
}


// function for checking if the command contains pipe or not
int has_pipe(char *command) {
    // checking for the existence of | character
    char *found_pipe = strstr(command, "|");
    char *found_or = strstr(command, "||");
    return found_pipe != NULL && (found_or == NULL || found_pipe < found_or);
}


// function for checking if the command contains ||
int has_or_operator(char *command) {
    // working on the copy of the command
    char *command_copy = strdup(command);
    // splitting by splace
    char *sub_command_string = strtok(command_copy, " ");
    // continuously searching for ||
    while (sub_command_string != NULL) {
        // searching for the || operator
        if (strcmp(sub_command_string, "||") == 0) {
            free(command_copy);
            return 1;
        }
        // resetting tokenizer for next loop
        sub_command_string = strtok(NULL, " ");
    }
    free(command_copy);
    return 0;
}


// function for checking if the command contains >
int has_output_redirect(char *command) {
    char *found = strstr(command, ">");
    return found != NULL && (found[1] != '>' || strlen(found) == 1);
}


// function for checking if the command contains >>
int has_append_redirect(char *command) {
    return strstr(command, ">>") != NULL;
}


// function for checking if the command contains #
int has_hash(char *command) {
    return strchr(command, '#') != NULL;
}


// function for checking if the command contains <
int has_input_redirect(char *command) {
    return strchr(command, '<') != NULL;
}


// function for checking if the command contains <<
int has_and_operator(char *command) {
    char *found = strstr(command, "&&");
    return found != NULL && (found[2] == '\0' || found[2] == ' ');
}


// function for checking if the command contains &
int has_background_process(char *command) {
    int length = strlen(command);
    return length > 0 && command[length - 1] == '&';
}


// function for checking if the command contains ;
int has_sequential_execution(char *command) {
    return strchr(command, ';') != NULL;
}


// function for executing a command sequentially
void execute_command_sequence(const char *full_command) {
    char command[strlen(full_command) + 1];
    strcpy(command, full_command);

    char *args[strlen(full_command) / 2 + 1];
    int argc = 0;

    // splitting the command with space for tokenization
    char *sub_command_string = strtok(command, " ");
    while (sub_command_string != NULL) {
        // generating the array with all the command parts
        args[argc++] = sub_command_string;
        sub_command_string = strtok(NULL, " ");
    }
    // null terminating
    args[argc] = NULL;

    // creating child for running the command
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    // child process
    else if (pid == 0) {
        // executing the prepared command
        execvp(args[0], args);

        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        // waiting for the child
        waitpid(pid, &status, 0);
    }
}


// function for executing output redirection operator
void execute_output_redirection_command(const char *full_command, const char *output_file) {
    // forking to run command from child
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    // child process
    else if (pid == 0) {
        // opening the provided file to write in
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // using dup to redirect the STDOUT_FILENO to the file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(fd);

        // running the code with the command to write the data
        execlp("sh", "sh", "-c", full_command, NULL);

        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        // waiting for child process
        waitpid(pid, &status, 0);
    }
}


// function for executing output append redirection operator
void execute_output_append_redirection_command(const char *full_command, const char *output_file) {
    // forking to run command from child
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    // child process
    else if (pid == 0) {
        // opening the provided file to append in
        int fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // using dup to redirect the STDOUT_FILENO to the file
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(fd);

        // running the code with the command to write the data
        execlp("sh", "sh", "-c", full_command, NULL);

        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        // waiting for child process
        waitpid(pid, &status, 0);
    }
}


// function for executing input redirection operator
void execute_input_redirection_command(const char *full_command, const char *input_file) {
    // forking to run command from child
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    // child process
    else if (pid == 0) {

        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // using dup to redirect the STDOUT_FILENO to the file
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(fd);

        // running the code with the command to write the data
        execlp("sh", "sh", "-c", full_command, NULL);

        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        // waiting for child process
        waitpid(pid, &status, 0);
    }
}


// function for executing piped command
void execute_piped_commands(char *command) {
    char *commands[MAX_ALLOWED_PIPES];
    int num_pipes = 0;

    // tokenizing the command with |
    char *sub_command_string = strtok(command, "|");
    // checking if the pipe number is correct or not before storing in array
    // as long as tokens are found
    while (sub_command_string != NULL && num_pipes < MAX_ALLOWED_PIPES) {
        commands[num_pipes++] = sub_command_string;
        sub_command_string = strtok(NULL, "|");
    }

    // creating the pipe file descriptor array
    int pipes[num_pipes - 1][2];

    // generating the pipes with the file descriptors
    for (int i = 0; i < num_pipes - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    // using for loop to loop over all the subcommands
    for (int i = 0; i < num_pipes; i++) {
        // creating fork to execute command in child
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        // child block
        else if (pid == 0) {
            // closing all the pipe file descriptors that are not needed in current loop
            for (int j = 0; j < num_pipes - 1; j++) {
                if (j != i - 1 && j != i) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }

            // i != 0 is true if the current command is not the first command
            // in that case the input of current process will read from
            // previous process. Using dup to revert back to that output
            if (i != 0) {
                close(STDIN_FILENO);
                dup(pipes[i - 1][0]);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }

            // if current command is not the last command then the standard output is
            // redirected to write to the write end of this process
            if (i != num_pipes - 1) {
                close(STDOUT_FILENO);
                dup(pipes[i][1]);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            // calling the execute method for running the command
            execute_command_sequence(commands[i]);
            exit(EXIT_SUCCESS);
        }
    }

    // closing all the pipes finally
    for (int i = 0; i < num_pipes - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // waiting for child to finish
    for (int i = 0; i < num_pipes; i++) {
        wait(NULL);
    }
}


// function for replacing a specific character with a new character
void replace_character(char *str, char old_char, char new_char) {
    int i;
    int string_length = strlen(str);

    // searching for the character and replacing it
    for (i = 0; i < string_length; i++) {
        if (str[i] == old_char) {
            str[i] = new_char;
        }
    }
}


// for executing the commands in && || operation
int execute_command_sequence_status(const char *full_command) {
    // command array
    char command[strlen(full_command) + 1];
    // storing the full command to array
    strcpy(command, full_command);

    // array for storing command sub section
    char *args[strlen(full_command) / 2 + 1];
    int argc = 0;

    // tokenizing depending on space
    char *sub_command_string = strtok(command, " ");
    while (sub_command_string != NULL) {
        // storing all the subsection of the command in the array
        args[argc++] = sub_command_string;
        // resetting the tokenizer
        sub_command_string = strtok(NULL, " ");
    }
    args[argc] = NULL;

    // forking to run the command in child
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    // child block
    else if (pid == 0) {
        // executing the command
        execvp(args[0], args);

        perror("execvp");
        exit(EXIT_FAILURE);
    }
    // parent block
    else {
        // for storign the status sent by child
        int status;
        // waiting for the child to send status
        waitpid(pid, &status, 0);

        // returning the status
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return EXIT_FAILURE;
        }
    }
}


// function for getting the subcommands from the main command by || character
char** get_subcommands_by_or(char *command) {
    // creating the holder array
    char** subcommands_or = malloc(sizeof(char*) * 100);
    if (subcommands_or == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    char *start = command;
    char *end = command;
    int length = strlen(command);
    int i;

    // looping for the length of the command
    for (i = 0; i < length; i++) {
        // if || found
        if (command[i] == '|' && command[i + 1] == '|') {
            // calculating the sub command length
            int sub_len = end - start;
            // allocating memory for the subcommand
            subcommands_or[count] = malloc(sub_len + 1);
            if (subcommands_or[count] == NULL) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }
            // storing data to the array by copying
            strncpy(subcommands_or[count], start, sub_len);
            // null terminating
            subcommands_or[count][sub_len] = '\0';
            // removing any unnecessary & characters
            replace_character(subcommands_or[count], '&', ' ');
            count++;
            // moving the cursor by 2 point to read next to the special operator
            start = &command[i + 2];
            // resetting end pointer
            end = start;
        }
        // if && operator not ound then move to next
        else {
            end++;
        }
    }

    int sub_len = end - start;
    subcommands_or[count] = malloc(sub_len + 1);
    if (subcommands_or[count] == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    strncpy(subcommands_or[count], start, sub_len);
    subcommands_or[count][sub_len] = '\0';
    replace_character(subcommands_or[count], '&', ' ');
    count++;

    subcommands_or[count] = NULL;

    return subcommands_or;
}


// function for getting the subcommands from the main command by && character
char** get_subcommands(char *command) {
    // creating the holder array
    char** subcommands = malloc(sizeof(char*) * 100);
    if (subcommands == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    char *start = command;
    char *end = command;
    int length = strlen(command);
    int i;

    // looping for the length of the command
    for (i = 0; i < length; i++) {
        // if && found
        if (command[i] == '&' && command[i + 1] == '&') {
            // calculating the sub command length
            int sub_command_len = end - start;
            // allocating memory for the subcommand
            subcommands[count] = malloc(sub_command_len + 1);
            if (subcommands[count] == NULL) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }
            // storing data to the array by copying
            strncpy(subcommands[count], start, sub_command_len);
            // null terminating
            subcommands[count][sub_command_len] = '\0';
            count++;

            // moving the cursor by 2 point to read next to the special operator
            start = &command[i + 2];
            // resetting end pointer
            end = start;
        }
        // if && operator not ound then move to next
        else {
            end++;
        }
    }

    // getting length of the substring
    int sub_len = end - start;
    // allocating memory
    subcommands[count] = malloc(sub_len + 1);
    if (subcommands[count] == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    // inserting the last subcommand to the array
    strncpy(subcommands[count], start, sub_len);
    // null terminating
    subcommands[count][sub_len] = '\0';
    count++;

    subcommands[count] = NULL;

    return subcommands;
}


// function for running the && || command
void handle_and_or(char *command){
    // storing the subcommands
    char** subcommands = get_subcommands(command);

    // for each subcommand separated by &&
    for (int i = 0; subcommands[i] != NULL; i++) {
        // get the subcommands separated by ||
        char **subcommands_or = get_subcommands_by_or(subcommands[i]);
        // for each subcommand
        for (int j = 0; subcommands_or[j] != NULL; j++) {
            // replacing all unnecessary garbage characters
            replace_character(subcommands_or[j], '|', ' ');
            replace_character(subcommands_or[j], '&', ' ');
            // executing the commands
            int status = execute_command_sequence_status(subcommands_or[j]);
            // if a set of commands connected by || is run, and one of them is successful
            // immediately it stops and subsequent commands have no need for execution
            if (status == EXIT_SUCCESS) {
                break;
            }
        }
        // freeing memory
        for (int k = 0; subcommands_or[k] != NULL; k++) {
            free(subcommands_or[k]);
        }
        free(subcommands_or);
    }

    for (int i = 0; subcommands[i] != NULL; i++) {
        free(subcommands[i]);
    }
    free(subcommands);
}


int main(int argc, char *argv[]) {
    // for storing user entered command
    char command[MAX_COMMAND_LENGTH];
    // shell prompt printing variable. if 1 then print shell prompt
    int print_prompt = 1;

    // if the program has already been run by NEWT then no need to print
    // the shell prompt text
    if (argc > 1 && strcmp(argv[1], "newt") == 0) {
        // setting the print_prompt to 0 so that no more shell prompt is printed
        print_prompt = 0;
    }

    // starting the infinite loop
    while (1) {
        // checking whether shell prompt needs to be printed or not
        if (print_prompt) {
            // printing the shell prompt
            printf("\nshell24$ ");
            // flushing to show the prompt immediately
            fflush(stdout);
        }

        // checks whether command reading was successful
        if (fgets(command, sizeof(command), stdin) == NULL) {
            fprintf(stderr, "Error reading command\n");
            continue;
        }

        // trimming the command
        command[strcspn(command, "\n")] = '\0';

        // if user enters NEWT then open a new terminal
        if (strcmp(command, "newt") == 0) {
            // forking to run the new terminal opening code
            if (fork() == 0) {
                // inside child executing the xterm command
                execlp("xterm", "xterm", "-e", "./shell24", "newt", NULL);
                exit(EXIT_SUCCESS);
            }
        }
        // if not newt then work with the custom commands
        else {

            // # Text file (.txt) concatenation
            // checking which special command does the string contain
            if (has_hash(command)) {
                // Splitting command by # operator
                char **hash_commands = split_by_operator(command, "#");
                // checking the total number of allowed special command
                if (special_character_count > 6) {
                    printf("Maximum 5 # can be handled at a time\n");
                } else {
                    // running the file concatenation meethod
                    concatenate_files(hash_commands);
                }
                // Freeing the memory
                free(hash_commands);
            }

            // | Piping
            // checking which special command does the string contain
            else if (has_pipe(command)) {
                // Splitting command by | operator
                char **pipe_commands = split_by_operator(command, "|");
                // checking the total number of allowed special command
                if (special_character_count > 7) {
                    printf("Maximum 6 | can be handled at a time\n");
                } else {
                    // running the piped command executing function
                    execute_piped_commands(command);
                }
                // Freeing the memory
                free(pipe_commands);
            }

            // DONE > Redirection
            // checking which special command does the string contain
            else if (has_output_redirect(command)) {
                // Splitting command by > operator
                char **output_redirection_commands = split_by_operator(command, ">");
                // checking the total number of allowed special command
                if (special_character_count > 2) {
                    printf("Maximum 1 > can be handled at a time\n");
                } else if (special_character_count == 2) {
                    // running the output redirection method
                    execute_output_redirection_command(
                    output_redirection_commands[0],
                    output_redirection_commands[1]
                    );
                } else {
                    printf("Invalid output redirection command");
                }
                // Freeing the memory
                free(output_redirection_commands);
            }

            // DONE >> Redirection
            // checking which special command does the string contain
            else if (has_append_redirect(command)) {
                // Splitting command by >> operator
                char **output_append_redirection_commands = split_by_operator(command, ">");
                // checking the total number of allowed special command
                if (special_character_count > 2) {
                    printf("Maximum 1 >> can be handled at a time\n");
                } else if (special_character_count == 2) {
                    // running the append redirection method
                    execute_output_append_redirection_command(
                            output_append_redirection_commands[0],
                            output_append_redirection_commands[1]
                    );
                } else {
                    printf("Invalid output append redirection command");
                }
                // Freeing the memory
                free(output_append_redirection_commands);
            }

            // DONE < Redirection
            // checking which special command does the string contain
            else if (has_input_redirect(command)) {
                // Splitting command by < operator
                char **input_redirection_commands = split_by_operator(command, "<");
                // checking the total number of allowed special command
                if (special_character_count > 2) {
                    printf("Maximum 1 < can be handled at a time\n");
                } else if (special_character_count == 2) {
                    // running the input redirection command
                    execute_input_redirection_command(
                            input_redirection_commands[0],
                            input_redirection_commands[1]
                    );
                } else {
                    printf("Invalid input redirection command");
                }
                // Freeing the memory
                free(input_redirection_commands);
            }

            // DONE && Conditional Execution
            // checking which special command does the string contain
            else if (has_and_operator(command)) {
                int total_operators = count_operators(command);

                // checking the total number of allowed special command
                if (total_operators > 5) {
                    printf("Maximum 5 || or && can be handled at a time\n");
                } else {
                    //running the handle || && command
                    handle_and_or(command);
                }
            }

            // DONE || Conditional Execution
            // checking which special command does the string contain
            else if (has_or_operator(command)) {
                int total_operators = count_operators(command);

                // checking the total number of allowed special command
                if (total_operators > 5) {
                    printf("Maximum 5 || or && can be handled at a time\n");
                } else {
                    //running the handle || && command
                    handle_and_or(command);
                }
            }

            // DONE & Background Processing
            // checking which special command does the string contain
            else if (has_background_process(command)) {
                // Splitting command by space
                char **bg_commands = split_by_operator(command, " ");
                // calling the run on background method
                run_in_background(bg_commands[0]);
                // Freeing the memory
                free(bg_commands);

            }
            else if (strcmp(command, "fg") == 0) {
                // bringing the last process that was sent to background to foreground
                bring_to_foreground();
            }

            // DONE ; Sequential execution
            // checking which special command does the string contain
            else if (has_sequential_execution(command)) {
                // Splitting command by ; operator
                char **sequential_commands = split_by_operator(command, ";");

                // checking the total number of allowed special command
                if (special_character_count > 5) {
                    printf("Maximum 5 ; can be handled at a time\n");
                } else {
                    for (int i = 0; i < special_character_count; i++) {
                        // Splitting command by space
                        char **specific_command = split_by_space_operator(sequential_commands[i], " ");

                        // checking the total number of allowed special command
                        if (special_space_count > 6) {
                            printf("Maximum 5 args can be handled at a time\n");
                            break;
                        } else {
                            // running the sequential execution function
                            execute_command_sequence(sequential_commands[i]);
                        }
                        // Freeing the memory
                        free(specific_command);
                    }
                }
                // Freeing the memory
                free(sequential_commands);
            }

            // DONE Plain command
            else {
                printf("Plain command: %s\n", command);
                // running the plain command
                execute_command_sequence(command);
            }
        }
        // resetting print_prompt so that after running the command
        // the shell prompt is printed again
        print_prompt = 1;
    }
    return 0;
}

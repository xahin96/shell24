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

void run_in_background (char *command) {
    int pid = fork();
    if ( pid == 0 ) {
        setsid();
        execlp(command, command, NULL);
        perror("execlp");
    }
    else if (pid > 0) {
        background_pid = pid;
        kill(pid, SIGSTOP);
    } else {
        printf("Fork failed\n");
    }
}

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
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        char *args[special_character_count + 2];
        args[0] = "cat";
        for (int i = 0; i < special_character_count; i++) {
            args[i + 1] = strdup(fileNames[i]);
        }
        args[special_character_count + 1] = NULL;

        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

char **split_by_operator(const char *command, const char *special_character) {
    special_character_count = 0;
    char *token;
    char *command_copy = strdup(command);
    char **commands = malloc(MAX_COMMAND_LENGTH * sizeof(char *));

    token = strtok(command_copy, special_character);
    while (token != NULL) {
        commands[special_character_count++] = strdup(token);
        token = strtok(NULL, special_character);
    }
    commands[special_character_count] = NULL;
    free(command_copy);
    return commands;
}

char **split_by_space_operator(const char *command, const char *special_character) {
    special_space_count = 0;
    char *token;
    char *command_copy = strdup(command);
    char **commands = malloc(MAX_COMMAND_LENGTH * sizeof(char *));

    token = strtok(command_copy, special_character);
    while (token != NULL) {
        commands[special_space_count++] = strdup(token);
        token = strtok(NULL, special_character);
    }
    commands[special_space_count] = NULL;

    free(command_copy);
    return commands;
}

int count_operators(char *str) {
    int count = 0;
    char *ptr = str;
    while ((ptr = strstr(ptr, "||")) != NULL) {
        count++;
        ptr += 2;
    }
    ptr = str;
    while ((ptr = strstr(ptr, "&&")) != NULL) {
        count++;
        ptr += 2;
    }
    return count;
}

int has_pipe(char *command) {
    char *found_pipe = strstr(command, "|");
    char *found_or = strstr(command, "||");
    return found_pipe != NULL && (found_or == NULL || found_pipe < found_or);
}

int has_or_operator(char *command) {
    char *command_copy = strdup(command);
    char *token = strtok(command_copy, " ");
    while (token != NULL) {
        if (strcmp(token, "||") == 0) {
            free(command_copy);
            return 1;
        }
        token = strtok(NULL, " ");
    }
    free(command_copy);
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
    char command[strlen(full_command) + 1];
    strcpy(command, full_command);

    char *args[strlen(full_command) / 2 + 1];
    int argc = 0;

    char *token = strtok(command, " ");
    while (token != NULL) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        execvp(args[0], args);

        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

void execute_output_redirection_command(const char *full_command, const char *output_file) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        printf("output_file: %s\n", output_file);
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(fd);

        execlp("sh", "sh", "-c", full_command, NULL);

        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

void execute_output_append_redirection_command(const char *full_command, const char *output_file) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        int fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(fd);

        execlp("sh", "sh", "-c", full_command, NULL);

        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

void execute_input_redirection_command(const char *full_command, const char *input_file) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {

        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(fd);

        execlp("sh", "sh", "-c", full_command, NULL);

        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

void execute_piped_commands(char *command) {
    char *commands[MAX_PIPES];
    int num_pipes = 0;

    char *token = strtok(command, "|");
    while (token != NULL && num_pipes < MAX_PIPES) {
        commands[num_pipes++] = token;
        token = strtok(NULL, "|");
    }

    int pipes[num_pipes - 1][2];

    for (int i = 0; i < num_pipes - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_pipes; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            for (int j = 0; j < num_pipes - 1; j++) {
                if (j != i - 1 && j != i) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }
            }

            if (i != 0) {
                close(STDIN_FILENO);
                dup(pipes[i - 1][0]);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }

            if (i != num_pipes - 1) {
                close(STDOUT_FILENO);
                dup(pipes[i][1]);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            execute_command_sequence(commands[i]);
            exit(EXIT_SUCCESS);
        }
    }

    for (int i = 0; i < num_pipes - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

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
    char command[strlen(full_command) + 1];
    strcpy(command, full_command);

    char *args[strlen(full_command) / 2 + 1];
    int argc = 0;

    char *token = strtok(command, " ");
    while (token != NULL) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    } else if (pid == 0) {
        execvp(args[0], args);

        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return EXIT_FAILURE;
        }
    }
}

char** get_subcommands_by_or(char *command) {
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

    for (i = 0; i < length; i++) {
        if (command[i] == '|' && command[i + 1] == '|') {
            int sub_len = end - start;
            subcommands_or[count] = malloc(sub_len + 1);
            if (subcommands_or[count] == NULL) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }
            strncpy(subcommands_or[count], start, sub_len);
            subcommands_or[count][sub_len] = '\0';
            replaceCharacter(subcommands_or[count], '&', ' ');
            count++;

            start = &command[i + 2];
            end = start;
        } else {
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
    replaceCharacter(subcommands_or[count], '&', ' ');
    count++;

    subcommands_or[count] = NULL;

    return subcommands_or;
}

char** get_subcommands(char *command) {
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

    for (i = 0; i < length; i++) {
        if (command[i] == '&' && command[i + 1] == '&') {
            int sub_len = end - start;
            subcommands[count] = malloc(sub_len + 1);
            if (subcommands[count] == NULL) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }
            strncpy(subcommands[count], start, sub_len);
            subcommands[count][sub_len] = '\0';
            count++;

            start = &command[i + 2];
            end = start;
        } else {
            end++;
        }
    }

    int sub_len = end - start;
    subcommands[count] = malloc(sub_len + 1);
    if (subcommands[count] == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    strncpy(subcommands[count], start, sub_len);
    subcommands[count][sub_len] = '\0';
    count++;

    subcommands[count] = NULL;

    return subcommands;
}

void handle_and_or(char *command){
    char** subcommands = get_subcommands(command);

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
    char command[MAX_COMMAND_LENGTH];
    int print_prompt = 1;

    if (argc > 1 && strcmp(argv[1], "newt") == 0) {
        print_prompt = 0;
    }

    while (1) {
        if (print_prompt) {
            printf("\nshell24$ ");
            fflush(stdout);
        }

        if (fgets(command, sizeof(command), stdin) == NULL) {
            fprintf(stderr, "Error reading command\n");
            continue;
        }

        command[strcspn(command, "\n")] = '\0';

        if (strcmp(command, "newt") == 0) {
            if (fork() == 0) {
                execlp("xterm", "xterm", "-e", "./shell24", "newt", NULL);
                exit(EXIT_SUCCESS);
            }
        }
        else {

            // DONE # Text file (.txt) concatenation
            if (has_hash(command)) {
                //a.txt#b.txt#c.txt#d.txt#e.txt#f.txt#g.txt
                // Split command by OR operator
                char **hash_commands = split_by_operator(command, "#");
                if (special_character_count > 6) {
                    printf("Maximum 5 # can be handled at a time\n");
                } else {
                    concatenate_files(hash_commands);
                }
                // Free memory allocated for command array
                free(hash_commands);
            }

            // | Piping
            else if (has_pipe(command)) {
                // cat z.txt | grep x | wc | wc | wc | wc | wc | wc
                char **pipe_commands = split_by_operator(command, "|");
                if (special_character_count > 7) {
                    printf("Maximum 6 | can be handled at a time\n");
                } else {
                    execute_piped_commands(command);
                }
                free(pipe_commands);
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
                free(input_redirection_commands);
            }

            // DONE && Conditional Execution
            else if (has_and_operator(command)) {
                int total_operators = count_operators(command);

                if (total_operators > 5) {
                    printf("Maximum 5 || or && can be handled at a time\n");
                } else {
                    handle_and_or(command);
                }
            }

            // DONE || Conditional Execution
            else if (has_or_operator(command)) {
                int total_operators = count_operators(command);

                if (total_operators > 5) {
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
                free(sequential_commands);
            }

            // DONE Plain command
            else {
                printf("Plain command: %s\n", command);
                execute_command_sequence(command);
            }
        }
        print_prompt = 1;
    }
    return 0;
}

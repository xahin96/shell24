#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMAND_LENGTH 100

pid_t background_pid;

void append_cwd () {

    // Get the old PATH
    char *oldPath = getenv("PATH");
//    printf("oldPath: %s\n", oldPath);

    // Get the path of current working directory
    char *cwd;
    char *buf;
    long size;
    size = pathconf(".", _PC_PATH_MAX);
    buf = (char *)malloc((size_t)size);
    cwd = getcwd(buf, (size_t)size);
//    printf("cwd: %s\n", cwd);

    // Combine the old Path and current working path
    char *combinedPath = malloc(strlen(oldPath) + strlen(cwd));
    sprintf(combinedPath, "%s:%s", oldPath, cwd);
//    printf("combinedPath: %s\n", combinedPath);

    // Set the combinedParh as the new Path
    setenv("PATH", combinedPath, 1);

    free(combinedPath);
}

int has_background_process(char *command) {
    int length = strlen(command);
    return length > 0 && command[length - 1] == '&';
}

void run_in_background(char *command) {
    printf("The program input by user will be run in the background\n");
    printf("Background process command: %s\n", command); // Print the command being executed

    int pid = fork();
    // child
    if (pid == 0) {
        printf("%d - %d - %d \n", getpid(), getppid(), getpgid(getpid()));
        setsid();
        printf("%d - %d - %d \n", getpid(), getppid(), getpgid(getpid()));
        printf("The child process will be differentiated and run in the background\n");
//        if (execlp(command, command, NULL) == -1) {
        if (execlp(command, command, NULL) == -1) {
            perror("execlp");
            exit(EXIT_FAILURE);
        }
    }
        // parent
    else if (pid > 0) {
        background_pid = pid;
        kill(pid, SIGSTOP);
        printf("Background process pid = %d\n", pid);
    } else {
        perror("fork");
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

int main(int argc, char *argv[]) {
//    append_cwd();
    char command[MAX_COMMAND_LENGTH];
    int print_prompt = 1; // Flag to control prompt printing

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
        } else {
            if (has_background_process(command)) {
                command[strlen(command) - 1] = '\0'; // Remove the '&' character
                run_in_background(command);
            } else if (strcmp(command, "fg") == 0) {
                bring_to_foreground();
            }
        }
        print_prompt = 1;
    }
    return 0;
}

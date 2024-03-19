#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int i = 0;

char ** command_splitter(char *command) {
    i = 0;
    char **args = (char **)malloc(100 * sizeof(char *)); // Allocate memory for args array

    if (args == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Tokenize the command string
    char *token = strtok(command, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL; // Null-terminate the argument list

    return args;
}

int main() {
    char str[] = "grep key";

    char **tokens = command_splitter(str);

    for(int j = 0; j < i; j++) {
        printf("%s\n", tokens[j]);
    }

    free(tokens); // Free dynamically allocated memory

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 100 // Define your maximum command length

char **split_by_and_or_operator(char *command, char *special_character1, char *special_character2) {
    int special_and_or_count = 0;
    char *token;
    char **commands = malloc(MAX_COMMAND_LENGTH * sizeof(char *));

    // Split command by "||" and store each command in the array
    token = strtok(command, special_character1);
    while (token != NULL) {
        commands[special_and_or_count++] = token;
        token = strtok(NULL, special_character1);
    }
    commands[special_and_or_count] = NULL; // Null-terminate the array
    return commands;
}

void print_commands(char **commands) {
    for (int i = 0; commands[i] != NULL; i++) {
        printf("%d - %s\n", i, commands[i]);
    }
}

int main() {
    char command[] = "a || b || c && d";
    char *special_character1 = "||";
    char *special_character2 = "&&";

    printf("AND operator found in command: %s\n", command);
    char **result = split_by_and_or_operator(command, special_character1, special_character2);

    // Print the result
    print_commands(result);

    // Free allocated memory
    free(result);

    return 0;
}

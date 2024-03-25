#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() {
    printf("Starting bg_process in the run_in_background...\n");
    system("./bg_process &");

    while (1) {
        char command[100];
        printf("Enter a command (fg to bring to bring_to_foreground, q to quit): ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0'; // Remove trailing newline

        if (strcmp(command, "fg") == 0) {
            printf("Bringing run_in_background process to the bring_to_foreground...\n");
            system("fg");
        } else if (strcmp(command, "q") == 0) {
            // Quit the program
            break;
        } else {
            printf("Unknown command\n");
        }
    }

    return 0;
}

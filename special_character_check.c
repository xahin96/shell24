#include <stdio.h>
#include <string.h>

int main() {
    char str[100];
    int len, i;
    int count[256] = {0};
    char *special_chars[] = {"#", "|", "<", ">", ">>", "&&", "||", ";"};
    int num_special_chars = sizeof(special_chars) / sizeof(special_chars[0]);
    int special_char_count = 0;

    printf("Enter a command: ");
    fgets(str, sizeof(str), stdin);
    len = strlen(str);

    // Count occurrences of each character
    for (i = 0; i < len; i++) {
        count[(int)str[i]]++;
    }

    // Count occurrences of special characters
    for (i = 0; i < num_special_chars; i++) {
        char *pos = strstr(str, special_chars[i]);
        while (pos != NULL) {
            special_char_count++;
            pos = strstr(pos + 1, special_chars[i]);
        }
    }
    if (special_char_count == 0) {
        printf("No special character found in the command\n");
    } else if (special_char_count == 1) {
        printf("Special character found in the command\n");
    } else {
        printf("Multiple Special characters found in the command\n");
    }

    return 0;
}

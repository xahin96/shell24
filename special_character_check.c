#include <stdio.h>
#include <string.h>

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

int main() {
    char str[] = "a || b && c || d && e || f";
    int total_operators = count_operators(str);
    printf("Total number of occurrences of '||' and '&&': %d\n", total_operators);
    return 0;
}

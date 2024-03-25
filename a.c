#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> // Include fcntl.h for open function

int main() {
    while (1){
//        int x;
//        printf("Input number: ");
//        scanf("%d", &x);
//        printf("%d * 5 is: %d\n", x, x*5);
        for(int i = 0 ; i < 20 ; i++)
        {
            printf("bg\n");
            sleep(1);
        }
    }
}

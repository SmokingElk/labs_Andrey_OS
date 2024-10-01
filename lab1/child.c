#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

bool isStringCorrect (char *str, int length) {
    if (length < 1) return false;
    return str[length - 1] == '.' || str[length - 1] == ';';
}

int main () {
    char str[30];
    char letter;
    int i = 0;
    int errorCode;

    while (1) {
        while (read(0, &letter, sizeof(letter))) {
            str[i++] = letter;
            if (letter == '\0') break;
        }

        i--;
        
        if (isStringCorrect(str, i)) {
            write(1, str, i);
            write(1, "\n", sizeof(char));
            errorCode = 0;
        } else errorCode = 1;

        if (write(2, &errorCode, sizeof(errorCode)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        i = 0;
    }
    
    exit(EXIT_SUCCESS);
}

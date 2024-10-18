#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define BUFSIZ 1024

void reverse_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char tmp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = tmp;
    }
} 

int main() {
    char buf[BUFSIZ];

    while (1) {
        ssize_t readed = read(0, buf, BUFSIZ); 
        if (readed <= 0) { // 
            exit(EXIT_SUCCESS);
        }
        buf[readed - 1] = '\0'; 

        if (strlen(buf) == 0) {
            break;
        }

        reverse_string(buf); 
        write(1, buf, strlen(buf));
        write(1, "\n", 1);
    }

    return 0;
}

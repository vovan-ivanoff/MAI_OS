// программа для генерирования больших бесполезных файлов которые идентифицируют себя как строки...

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
int main(){
    char* buf = malloc(BUFSIZ*8);
    int fd = open("input_c.txt", O_WRONLY|O_CREAT);
    int idx =0;
    buf[idx]=97;
    for(long long i = 0; i <= 5000000000; i++){
        char res = (i % 4) + '0';
        buf[++idx]= res;
        if(idx == BUFSIZ*8) {
            write(fd, buf, BUFSIZ*8);
            idx = 0;
            // printf("64k block written\n");
        }
    }
    printf("%d written",write(fd, buf, idx));
    buf[0]='4';
    buf[1]='\0';
    write(fd, buf, 2);
    close(fd);
    free(buf);
    return 0;
}
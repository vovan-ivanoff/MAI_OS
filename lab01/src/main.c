#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>


#define BUFSIZ 1024


#define ECPIPE "Error creating pipe\n"
#define ECHILD1 "Error creating process 1\n"
#define EFCHILD1 "Error opening file for child1\n"
#define EECHILD1 "Exec error for child1\n"
#define ECHILD2 "Error creating process 2\n"
#define EFCHILD2 "Error opening file for child2\n"
#define EECHILD2 "Exec error for child2\n"
#define ERLINE "Error reading line\n"


int main() {
    int pipe1[2], pipe2[2];
    char buffer[BUFSIZ];
    int count = 1;
    char filename1[BUFSIZ];
    char filename2[BUFSIZ];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        write(2, ECPIPE, strlen(ECPIPE));
        exit(EXIT_FAILURE);
    }

    write(1, "Enter a filename for child1: ", 30);
    read(0, filename1, BUFSIZ);
    filename1[strcspn(filename1, "\n")] = '\0'; 

    write(1, "Enter a filename for child2: ", 30);
    read(0, filename2, BUFSIZ);
    filename2[strcspn(filename2, "\n")] = '\0'; 


    pid_t pid1 = fork();
    if (pid1 == -1) {
        write(2, ECHILD1, strlen(ECHILD1));
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        int fd1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd1 == -1) {
            write(2, EFCHILD1, strlen(EFCHILD1));
            exit(EXIT_FAILURE);
        }

        close(pipe1[1]); 
        dup2(pipe1[0], 0); 
        dup2(fd1, 1); 
        close(pipe1[0]);
        close(fd1);

        execl("./child", "child", NULL); 
        write(2, "exec error for child1.\n", 24);
        exit(EXIT_FAILURE);
    }


    pid_t pid2 = fork();
    if (pid2 == -1) {
        write(2, ECHILD2, strlen(ECHILD2));
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        int fd2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd2 == -1) {
            write(2, EFCHILD2, strlen(EFCHILD2));
            exit(EXIT_FAILURE);
        }

        close(pipe2[1]); 
        dup2(pipe2[0], 0); 
        dup2(fd2, 1); 
        close(pipe2[0]);
        close(fd2);

        execl("./child", "child", NULL); 
        write(2, EECHILD2, strlen(EECHILD2));
        exit(EXIT_FAILURE);
    }
    close(pipe1[0]); 
    close(pipe2[0]);

    while (1) {
        write(1, "Enter line: ", 13);
        ssize_t bytes_read = read(0, buffer, BUFSIZ);
        if (bytes_read == 0) { // EOF
            write(pipe2[1], "\0", 1);
            write(pipe1[1], "\0", 1);
            exit(EXIT_SUCCESS);
        } else if (bytes_read == -1){
            write(2, ERLINE, strlen(ERLINE));
            exit(EXIT_FAILURE);
        }
        buffer[bytes_read - 1] = '\0'; 

        if (strlen(buffer) == 0) {
            break;
        }

        if (strlen(buffer) > 10) {
            write(pipe2[1], buffer, bytes_read);
        } else {
            write(pipe1[1], buffer, bytes_read);
        }
    }
    close(pipe1[1]);
    close(pipe2[1]);
    return 0;
}
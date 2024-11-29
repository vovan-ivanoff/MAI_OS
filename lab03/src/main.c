#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define SHM_SIZE 1024
#define SEM_KEY1 1234
#define SEM_KEY2 5678

void P(int semid)
{
    struct sembuf p = {0, -1, 0};
    semop(semid, &p, 1);
}

void V(int semid)
{
    struct sembuf v = {0, 1, 0};
    semop(semid, &v, 1);
}

int main()
{
    char buffer[SHM_SIZE];
    char filename1[BUFSIZ];
    char filename2[BUFSIZ];
    write(1, "Enter a filename for child1: ", 30);
    read(0, filename1, BUFSIZ);
    filename1[strcspn(filename1, "\n")] = '\0';

    write(1, "Enter a filename for child2: ", 30);
    read(0, filename2, BUFSIZ);
    filename2[strcspn(filename2, "\n")] = '\0';

    int fd1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd1 == -1)
    {
        write(2, "Error Opening File 1", 20);
        exit(EXIT_FAILURE);
    }
    int fd2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd2 == -1)
    {
        close(fd1);
        write(2, "Error Opening File 2", 20);
        exit(EXIT_FAILURE);
    }

    // Создание семафоров
    int semid1 = semget(SEM_KEY1, 1, IPC_CREAT | 0666);
    semctl(semid1, 0, SETVAL, 1);
    int semid2 = semget(SEM_KEY2, 1, IPC_CREAT | 0666);
    semctl(semid2, 0, SETVAL, 1);
    if (semid1 == -1 || semid2 == -1)
    {
        close(fd1);
        close(fd2);
        write(2, "Error in Semget", 15);
        exit(1);
    }
    // Создание разделяемой памяти
    int shmid1 = shmget(1234, SHM_SIZE, IPC_CREAT | 0666);
    char *shmemes = shmat(shmid1, NULL, 0);

    if (shmemes == (char *)-1)
    {
        close(fd1);
        close(fd2);
        semctl(semid1, 0, IPC_RMID);
        semctl(semid2, 0, IPC_RMID);
        write(2, "Error in Shmat", 14);
        exit(1);
    }

    // Создаём дочерний процесс
    pid_t pid = fork();
    if (pid == -1)
    {
        close(fd1);
        close(fd2);
        shmdt(shmemes);
        shmctl(shmid1, IPC_RMID, NULL);
        semctl(semid1, 0, IPC_RMID);
        semctl(semid2, 0, IPC_RMID);
        write(2, "Error in Child1", 15);
        exit(1);
    }
    if (pid == 0)
    {
        dup2(fd1, 1);
        close(fd1);
        // Дочерний процесс 1
        execlp("./child", "./child", "1234", (char *)NULL);
        perror("execlp failed");
        exit(1);
    }
    pid = fork();
    if (pid == -1)
    {
        close(fd1);
        close(fd2);
        shmdt(shmemes);
        shmctl(shmid1, IPC_RMID, NULL);
        semctl(semid1, 0, IPC_RMID);
        semctl(semid2, 0, IPC_RMID);
        write(2, "Error in Child1", 15);
        exit(1);
    }
    if (pid == 0)
    {
        dup2(fd2, 1);
        close(fd2);
        execlp("./child", "./child", "5678", (char *)NULL);
        perror("execlp failed");
        exit(1);
    }
    // Записываем данные в общую память
    while (1)
    {
        write(1, "Enter line: ", 13);
        ssize_t bytes_read = read(0, buffer, SHM_SIZE);
        if (bytes_read == 0)
        { // EOF
            break;
        }
        else if (bytes_read == -1)
        {
            write(2, "Error reading Line", 18);
            close(fd1);
            close(fd2);
            shmdt(shmemes);
            shmctl(shmid1, IPC_RMID, NULL);
            semctl(semid1, 0, IPC_RMID);
            semctl(semid2, 0, IPC_RMID);
            exit(1);
        }
        buffer[bytes_read - 1] = '\0';

        if (strlen(buffer) == 0)
        {
            break;
        }

        if (strlen(buffer) > 10)
        {
            // write(pipe2[1], buffer, bytes_read);
            semctl(semid2, 0, SETVAL, 1);
            strcpy(shmemes, buffer);
            semctl(semid2, 0, SETVAL, 0);
        }
        else
        {
            // P(semid); // ждем 0
            semctl(semid1, 0, SETVAL, 1);
            strcpy(shmemes, buffer);
            semctl(semid1, 0, SETVAL, 0);
        }
    }
    // Удаление разделяемой памяти и семафоров
    P(semid1);
    P(semid2); // ждем пока семафоры поднимутся в 1
    memset(shmemes, 0, SHM_SIZE);
    semctl(semid1, 0, SETVAL, 0);
    semctl(semid2, 0, SETVAL, 0); // Шлем сигнал с пустым буфером == завершение
    shmdt(shmemes);
    shmctl(shmid1, IPC_RMID, NULL);
    semctl(semid1, 0, IPC_RMID);
    semctl(semid2, 0, IPC_RMID);

    return 0;
}

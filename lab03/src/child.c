#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>

#define SHM_SIZE 1024

void reverse_string(char *str)
{
    // sleep(2);
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++)
    {
        char tmp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = tmp;
    }
}

void P(int semid) // ождидаем семафор
{
    struct sembuf p = {0, 0, 0};
    semop(semid, &p, 1);
}

int is_digit(char c)
{
    return (c >= '0' && c <= '9');
}
int str_to_i(char *str)
{
    int buf = 0;
    int len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (!is_digit(str[i]))
            return -1;
        buf *= 10;
        buf += (str[i] - '0');
    }
    return buf;
}

int main(int argc, char **argv)
{
    // write(1, argv[1], strlen(argv[1]));
    int id = str_to_i(argv[1]);
    // Получаем семафор
    int semid = semget(id, 1, 0666);
    // Получаем доступ к общей памяти
    int shmid = shmget(1234, SHM_SIZE, 0666);
    char *shared_memory = shmat(shmid, NULL, 0);
    P(semid);
    while (strlen(shared_memory) > 0)
    {
        semctl(semid, 0, SETVAL, 1);
        reverse_string(shared_memory);
        write(1, shared_memory, strlen(shared_memory));
        write(1, "\n", 1);
        memset(shared_memory, 0, strlen(shared_memory));
        // semctl(semid, 0, SETVAL, 2);
        P(semid);
    }
    close(1);
    return 0;
}
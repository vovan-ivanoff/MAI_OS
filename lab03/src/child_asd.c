#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>

#define SHM_SIZE 1024
#define SEM_KEY 1234
#define SHM_KEY 5678

void P(int semid) {
    struct sembuf p = {0, -1, 0};
    semop(semid, &p, 1);
}

void V(int semid) {
    struct sembuf v = {0, 1, 0};
    semop(semid, &v, 1);
}

int main(int argc, char* argv[]) {
    // Получаем семафор
    int semid = semget(SEM_KEY, 1, 0666);
    // Получаем доступ к общей памяти
    int shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    char *shared_memory = shmat(shmid, NULL, 0);
    
    if (shared_memory == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    // Читаем данные из общей памяти
    P(semid);
    printf("Child: Read from shared memory: %s\n", shared_memory);
    V(semid);

    // Изменяем данные в общей памяти
    P(semid);
    strcpy(shared_memory, "Hello from child!");
    V(semid);

    printf("Child: Written to shared memory.\n");

    shmdt(shared_memory);
    return 0;
}

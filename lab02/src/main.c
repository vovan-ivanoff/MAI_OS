#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAX_THREADS 10 // Максимальное количество потоков

typedef struct
{
    char *haystack;  // Основная строка
    char *needle;    // Подстрока для поиска
    long long start; // Начальный индекс для поиска
    long long end;   // Конечный индекс для поиска
    int thread_id;   // ID потока
} ThreadData;

void reverse_str(char *str)
{
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++)
    {
        char tmp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = tmp;
    }
}

void print_ll(long long num, int fd)
{
    char buf[256];
    int idx = 0;
    if (num == 0)
    {
        buf[idx++] = '0';
    }
    else
    {
        while (num)
        {
            buf[idx++] = num % 10 + '0';
            num /= 10;
        }
    }
    buf[idx] = 0;
    reverse_str(buf);
    write(fd, buf, idx);
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

pthread_mutex_t mutex;
long long found_position = -1; // Позиция найденной подстроки
int res[MAX_THREADS];

void *search_substring(void *arg)
{
    ThreadData *data = (ThreadData *)arg;

    for (long long i = data->start; i <= data->end - strlen(data->needle); i++)
    { // ищем дальше нашец границы на длинну шаблона
        if (strncmp(&data->haystack[i], data->needle, strlen(data->needle)) == 0)
        {
            pthread_mutex_lock(&mutex);
            if (found_position == -1)
            {                       // Если еще не нашли
                found_position = i; // Записываем позицию
                res[data->thread_id] = 1;
                // printf("Подстрока найдена в потоке %d на позиции %lld\n", data->thread_id, found_position);
            }
            pthread_mutex_unlock(&mutex);
            return NULL; // Прерываем поиск, если уже нашли
        }
    }
    res[data->thread_id] = 0;
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        write(STDERR_FILENO, "Wrong number of args!", 21);
        return -1;
    }
    int num_threads = MAX_THREADS;
    if (!(strcmp(argv[2], "-j")))
    {
        num_threads = str_to_i(argv[3]);
        if (num_threads < 0)
        {
            write(STDERR_FILENO, "Error reading number", 20);
            return -1;
        }
        if (num_threads > MAX_THREADS)
        {
            write(STDERR_FILENO, "Maximum thread count exceeded!", 30);
            return -1;
        }
    }
    else
    {
        write(STDERR_FILENO, "Unknown flag (use -j num)", 25);
        return -1;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        write(STDERR_FILENO, "Error opening file", 18);
        return -1;
    }
    struct stat st;
    if (fstat(fd, &st))
    {
        close(fd);
        write(STDERR_FILENO, "Error in stat", 13);
        return -1;
    }
    // char* haystack = mmap(NULL,1027,PROT_READ,(MAP_PRIVATE),fd, 9999998976);
    long long len_haystack = st.st_size; // strlen не работает на 10гигабайтной строке и падает в сегфолт :(
    char *haystack = mmap(NULL, len_haystack, PROT_READ, (MAP_PRIVATE), fd, 0);
    if (haystack == (char *)-1)
    {
        close(fd);
        write(STDERR_FILENO, "Error in MMAP", 13);
        return -1;
    }

    char *needle = "34";

    pthread_t threads[num_threads];

    ThreadData thread_data[num_threads];
    pthread_mutex_init(&mutex, NULL);

    long long chunk_size = (long long)len_haystack / num_threads;

    // Создаем потоки
    for (int i = 0; i < num_threads; i++)
    {
        thread_data[i].haystack = haystack;
        thread_data[i].needle = needle;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i == num_threads - 1) ? len_haystack : (i + 1) * chunk_size + strlen(needle);
        thread_data[i].thread_id = i;

        if(pthread_create(&threads[i], NULL, search_substring, (void *)&thread_data[i])){
            write(STDERR_FILENO, "Error creating thread!", 22);
            munmap(haystack, len_haystack);
            close(fd);
            return -1;
        };
    }

    // Ждем завершения всех потоков
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    if (found_position == -1)
    {
        write(1, "NOT FOUND\n", 10);
    }
    else
    {
        for (int i = 0; i < num_threads; i++)
        {
            if (res[i] == 1)
            {
                write(1, "found in thread: ", 17);
                print_ll(i, 1);
                write(1, " at position ", 13);
                print_ll(found_position, 1);
            }
        }
    }
    pthread_mutex_destroy(&mutex);
    munmap(haystack, len_haystack);
    close(fd);
    return 0;
}
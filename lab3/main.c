#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>

#define SHM_SIZE 1024
#define FILENAME_SIZE 30
#define BUFFER_SIZE 30
#define SHARED_SEG_NAME "/shared_memory"

typedef struct {
    char buffer[BUFFER_SIZE];
    int strsCount;
    int isEnd;
    pthread_mutex_t mutex;
} SharedData;

int main() {
    char filename[FILENAME_SIZE];
    printf("Input filename: ");
    scanf("%s", filename);

    int file = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (file == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(SHARED_SEG_NAME, O_CREAT | O_RDWR, 0644);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // выделение памяти по созданному дескриптору
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // получить указатель на область зашереной памяти
    SharedData *shared_data = (SharedData *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // сделать мьютекс используемым разными процессами через установку атрибута 
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared_data->mutex, &mutex_attr);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        if (dup2(file, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        char *const *null = NULL;
        if (execv("./child.out", null) == -1) {
            perror("execv error");
            return 1;
        }
    } else {
        close(file);
        char buffer[BUFFER_SIZE];
        shared_data->strsCount = 0;

        while (1) {
            printf("Input str to write it to file: ");
            scanf("%s", buffer);

            if (strcmp(buffer, "exit") == 0) break;

            pthread_mutex_lock(&shared_data->mutex);
            strcpy(shared_data->buffer, buffer);
            shared_data->strsCount++;
            pthread_mutex_unlock(&shared_data->mutex);

            sleep(1); // ждем пока пройдет проверка у child process

            pthread_mutex_lock(&shared_data->mutex);
            int isEnd = shared_data->isEnd;
            pthread_mutex_unlock(&shared_data->mutex);

            if (isEnd == 1) {
                printf("Error!\n");
            }
        }

        pthread_mutex_destroy(&shared_data->mutex);
        munmap(shared_data, SHM_SIZE);
        shm_unlink("/shared_memory");
        exit(EXIT_SUCCESS);
    }
}
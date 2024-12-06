#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdbool.h>

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

bool isStringCorrect(char *str, int length) {
    if (length < 1) return false;
    return str[length - 1] == '.' || str[length - 1] == ';';
}

int main() {
    int shm_fd = shm_open(SHARED_SEG_NAME, O_RDWR, 0644);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    SharedData *shared_data = (SharedData *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    int strsWritten = 0;

    while (1) {
        pthread_mutex_lock(&shared_data->mutex);
        char *str = shared_data->buffer;
        int length = strlen(str);
        if (isStringCorrect(str, length)) {
            if (strsWritten != shared_data->strsCount) {
                strsWritten = shared_data->strsCount;
                write(STDOUT_FILENO, str, length);
                write(STDOUT_FILENO, "\n", sizeof(char));
            }
            
            shared_data->isEnd = 0;
        } else {
            shared_data->isEnd = 1;
        }
        pthread_mutex_unlock(&shared_data->mutex);
        sleep(1); // ждем завершения чтения у родительского процесса 
    }

    munmap(shared_data, SHM_SIZE);
    exit(EXIT_SUCCESS);
}
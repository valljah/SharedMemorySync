#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/semaphore.h>
#include <unistd.h>
#include "defines.h"

sem_t producer_sem;
sem_t consumer_sem;

int main() {
    shm_unlink(SHARED_MEM_NAME);

    int shared_memory = shm_open(SHARED_MEM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    int file = ftruncate(shared_memory, PAYLOAD_OFFSET);
    void *mem_map;

    sem_init(&producer_sem, 1, 0);
    sem_init(&consumer_sem, 1, 0);
    create_mmap(shared_memory, file, &mem_map);

    sync_t *sync = (sync_t *) mem_map;
    
    *sync = (sync_t) {.new_work = consumer_sem, 
                      .new_result = producer_sem, 
                      .payload_offset = SYNC_OFFSET};
    
    payload_t *payload = (payload_t *) sync->payload_offset;

    int i = 1;
    int cnt = 0;
    while (cnt < MAX_CNT) {
        sem_wait(&producer_sem);
        printf("Result received from consumer: %d\n\n", payload->result);
        printf("Putting in %d to work on now\n", i);
        payload->work = i;
        i++;
        struct timespec sleep_duration = (struct timespec) {.tv_sec = 0, .tv_nsec = 100000000};
        nanosleep(&sleep_duration, NULL);
        sem_post(&consumer_sem);
        cnt++;
    }

    int nunmap_value = munmap(mem_map, PAYLOAD_OFFSET);
    if (nunmap_value == -1) {
        perror("Failure while unmapping memory");
    }

    sem_destroy(&producer_sem);
    sem_destroy(&consumer_sem);
    shm_unlink(SHARED_MEM_NAME);
}

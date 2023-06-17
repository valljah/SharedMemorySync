#include <pthread.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/semaphore.h>
#include "defines.h"

sem_t producer_sem;
sem_t consumer_sem;

int main() {
    int shared_memory = shm_open(SHARED_MEM_NAME, O_RDWR | O_CREAT, 0666);
    void *mem_map;

    create_mmap(shared_memory, 0, &mem_map);

    sync_t *sync = (sync_t *) mem_map;
    
    producer_sem = sync->new_result;
    consumer_sem = sync->new_work;
    
    payload_t *payload = (payload_t *) mem_map + sync->payload_offset;

    int cnt = 0;
    sem_post(&producer_sem);
    while (cnt < MAX_CNT) {
        sem_wait(&consumer_sem);
        printf("Number to work on received from producer: %d\n", payload->work);
        printf("Putting %d into result!\n\n", (payload->work*payload->work));
        payload->result = (payload->work*payload->work);
        struct timespec sleep_duration = (struct timespec) {.tv_sec = 0, .tv_nsec = 100000000};
        nanosleep(&sleep_duration, NULL);
        sem_post(&producer_sem);
        cnt++;
    }

    int nunmap_value = munmap(mem_map, BUFFER_SIZE);
    if (nunmap_value == -1) {
        perror("Failure while unmapping memory");
    }

    shm_unlink(SHARED_MEM_NAME);

    sem_destroy(&producer_sem);
    sem_destroy(&consumer_sem);
    // close_semaphores(consumer_sem, producer_sem);
}

#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include "fifo.h"

#define FIFO_KEY 0x1234

fifo_t *attacher_fifo() {
    int shmid = shmget(FIFO_KEY, sizeof(fifo_t), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return NULL;
    }

    fifo_t *fifo = (fifo_t *)shmat(shmid, NULL, 0);
    if (fifo == (void *)-1) {
        perror("shmat");
        return NULL;
    }

    return fifo;
}

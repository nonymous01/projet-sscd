#include <stdio.h>
#include "fifo.h"

void fifo_init(fifo_t *fifo) {
    fifo->front = 0;
    fifo->rear = -1;
    fifo->count = 0;
}

int fifo_enqueue(fifo_t *fifo, process_t proc) {
    if (fifo->count == MAX_TASKS) {
        return -1; // FIFO pleine
    }
    fifo->rear = (fifo->rear + 1) % MAX_TASKS;
    fifo->tasks[fifo->rear] = proc;
    fifo->count++;
    return 0;
}

int fifo_dequeue(fifo_t *fifo, process_t *proc) {
    if (fifo->count == 0) {
        return -1; // FIFO vide
    }
    *proc = fifo->tasks[fifo->front];
    fifo->front = (fifo->front + 1) % MAX_TASKS;
    fifo->count--;
    return 0;
}

void fifo_print(const fifo_t *fifo) {
    printf("FIFO contient %d processus:\n", fifo->count);
    int i = fifo->front;
    for (int c = 0; c < fifo->count; c++) {
        process_t p = fifo->tasks[i];
        printf("PID=%d, State=%d, Burst=%d\n", p.pid, p.state, p.burst_time);
        i = (i + 1) % MAX_TASKS;
    }
}

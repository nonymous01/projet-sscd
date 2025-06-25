#include "fifo.h"
#include <stdio.h>
#include <string.h>

// Initialise la file FIFO
void fifo_init(fifo_t *fifo) {
    fifo->front = 0;
    fifo->rear = -1;
    fifo->count = 0;
}

// Ajoute une tâche à la FIFO
int fifo_enqueue(fifo_t *fifo, process_t task) {
    if (fifo->count >= MAX_TASKS) {
        return -1; // file pleine
    }

    fifo->rear = (fifo->rear + 1) % MAX_TASKS;
    fifo->tasks[fifo->rear] = task;
    fifo->count++;
    return 0;
}

// Retire une tâche de la FIFO
int fifo_dequeue(fifo_t *fifo, process_t *task) {
    if (fifo->count == 0) {
        return -1; // file vide
    }

    *task = fifo->tasks[fifo->front];
    fifo->front = (fifo->front + 1) % MAX_TASKS;
    fifo->count--;
    return 0;
}

// Affiche la FIFO dans le terminal
void fifo_print(const fifo_t *fifo) {
    printf("Contenu de la FIFO (%d éléments):\n", fifo->count);
    int i = fifo->front;
    for (int c = 0; c < fifo->count; c++) {
        const process_t *p = &fifo->tasks[i];
        printf("  PID: %d | Cmd: %s | État: %d\n", p->pid, p->command, p->state);
        i = (i + 1) % MAX_TASKS;
    }
}

// ✅ Fonction ajoutée : vérifier si la FIFO est vide
int fifo_empty(const fifo_t *fifo) {
    return fifo->count == 0;
}

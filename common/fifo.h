#ifndef FIFO_H
#define FIFO_H

#include <time.h>  // pour time_t

#define MAX_TASKS 100

typedef enum {
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} state_t;

typedef struct {
    int pid;
    char user[32];
    char command[128];
    state_t state;             // ✅ remplacé le doublon
    int priority;
    int burst_time;
    int remaining_time;
    time_t enqueue_time;
    time_t start_time;
    time_t end_time;
    float cpu_usage;
    float mem_usage;
} process_t;

typedef struct {
    process_t tasks[MAX_TASKS];
    int front;  // indice de la tête (élément à dépiler)
    int rear;   // indice de la queue (élément le plus récent empilé)
    int count;  // nombre d’éléments dans la FIFO
} fifo_t;

// Prototypes des fonctions
void fifo_init(fifo_t *fifo);
int fifo_enqueue(fifo_t *fifo, process_t task);
int fifo_dequeue(fifo_t *fifo, process_t *task);
void fifo_print(const fifo_t *fifo);

#endif // FIFO_H

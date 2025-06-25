#ifndef FIFO_H
#define FIFO_H

#include <time.h>  // pour time_t

#define MAX_TASKS 100

// États possibles d’un processus
typedef enum {
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} state_t;

// Structure d’un processus
typedef struct {
    int pid;
    char user[32];
    char command[128];
    state_t state;
    int priority;
    int burst_time;
    int remaining_time;
    time_t enqueue_time;
    time_t start_time;
    time_t end_time;
    float cpu_usage;
    float mem_usage;
} process_t;

// File FIFO
typedef struct {
    process_t tasks[MAX_TASKS];
    int front;
    int rear;
    int count;
} fifo_t;

// Prototypes des fonctions FIFO
void fifo_init(fifo_t *fifo);
int fifo_enqueue(fifo_t *fifo, process_t task);
int fifo_dequeue(fifo_t *fifo, process_t *task);
void fifo_print(const fifo_t *fifo);
int fifo_empty(const fifo_t *fifo);  // ✅ PROTOTYPE AJOUTÉ ICI

#endif // FIFO_H

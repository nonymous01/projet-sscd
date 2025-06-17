#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "../common/fifo.h"
#include "ordonnanceur_fifo.h"

#define SHM_KEY 0x1234

void init_stats_json() {
    FILE *fp = fopen("ordonnanceur_output.json", "w");
    if (!fp) {
        perror("Erreur création du fichier JSON initial");
        return;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"tasks_in_queue\": [],\n");
    fprintf(fp, "  \"current_task\": null,\n");
    fprintf(fp, "  \"tasks_processed\": 0,\n");
    fprintf(fp, "  \"average_wait_time\": 0.00,\n");
    fprintf(fp, "  \"throughput\": 0.00\n");
    fprintf(fp, "}\n");

    fclose(fp);
}

void *ordonnanceur_fifo(void *arg) {
    printf("✅ Ordonnanceur FIFO lancé dans un thread\n");

    init_stats_json();

    int shmid = shmget(SHM_KEY, sizeof(fifo_t), 0666);
    if (shmid < 0) {
        perror("shmget ordonnanceur");
        pthread_exit(NULL);
    }

    fifo_t *fifo = (fifo_t *)shmat(shmid, NULL, 0);
    if (fifo == (void *)-1) {
        perror("shmat ordonnanceur");
        pthread_exit(NULL);
    }

    int total_tasks_processed = 0;
    double total_wait_time = 0.0;
    clock_t start_time = clock();

    while (1) {
        process_t task;
        int ret = fifo_dequeue(fifo, &task);
        if (ret == 0) {
            printf("Traitement tâche PID=%d\n", task.pid);
            total_tasks_processed++;

            sleep(1);

            clock_t now = clock();
            double elapsed = (double)(now - start_time) / CLOCKS_PER_SEC;
            total_wait_time += elapsed;

            double avg_wait = total_tasks_processed ? total_wait_time / total_tasks_processed : 0;
            double throughput = elapsed > 0 ? total_tasks_processed / elapsed : 0;

            printf("Tâche PID %d terminée\n", task.pid);
            printf("Tâches traitées : %d\n", total_tasks_processed);
            printf("Temps d'attente moyen : %.2f s\n", avg_wait);
            printf("Débit (throughput) : %.2f tâches/s\n\n", throughput);

            FILE *fp = fopen("ordonnanceur_output.json", "w");
            if (fp) {
                fprintf(fp, "{\n");
                fprintf(fp, "  \"tasks_in_queue\": [");
                for (int i = 0; i < fifo->count; i++) {
                    int idx = (fifo->front + i) % MAX_TASKS;
                    fprintf(fp, "%d", fifo->tasks[idx].pid);
                    if (i < fifo->count - 1) fprintf(fp, ", ");
                }
                fprintf(fp, "],\n");
                fprintf(fp, "  \"current_task\": %d,\n", task.pid);
                fprintf(fp, "  \"tasks_processed\": %d,\n", total_tasks_processed);
                fprintf(fp, "  \"average_wait_time\": %.2f,\n", avg_wait);
                fprintf(fp, "  \"throughput\": %.2f\n", throughput);
                fprintf(fp, "}\n");
                fclose(fp);
            }

        } else {
            printf("🕒 Pas de tâche à traiter, attente...\n");
            sleep(1);
        }
    }

    shmdt(fifo);
    pthread_exit(NULL);
}

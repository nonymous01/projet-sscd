#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "../common/fifo.h"
#include "ordonnanceur_fifo.h"

void update_json(fifo_t *fifo, int current_pid, int processed, double avg_wait, double throughput) {
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
        if (current_pid >= 0)
            fprintf(fp, "  \"current_task\": %d,\n", current_pid);
        else
            fprintf(fp, "  \"current_task\": null,\n");
        fprintf(fp, "  \"tasks_processed\": %d,\n", processed);
        fprintf(fp, "  \"average_wait_time\": %.2f,\n", avg_wait);
        fprintf(fp, "  \"throughput\": %.2f\n", throughput);
        fprintf(fp, "}\n");
        fclose(fp);
    }
}

void ordonnanceur_fifo(fifo_t *fifo) {
    int total_tasks_processed = 0;
    double total_wait_time = 0.0;
    clock_t start_time = clock();

    while (1) {
        if (fifo_empty(fifo)) {
            printf("🕒 File vide, ordonnanceur en attente...\n");
            sleep(1);
            continue;
        }

        // Mise à jour JSON avant traitement
        clock_t now_pre = clock();
        double elapsed_pre = (double)(now_pre - start_time) / CLOCKS_PER_SEC;
        double avg_wait_pre = total_tasks_processed ? total_wait_time / total_tasks_processed : 0;
        double throughput_pre = elapsed_pre > 0 ? total_tasks_processed / elapsed_pre : 0;
        update_json(fifo, -1, total_tasks_processed, avg_wait_pre, throughput_pre);

        // Traitement
        process_t task;
        fifo_dequeue(fifo, &task);
        printf("Traitement tâche PID=%d\n", task.pid);
        total_tasks_processed++;

        sleep(1); // simulation

        clock_t now = clock();
        double elapsed = (double)(now - start_time) / CLOCKS_PER_SEC;
        total_wait_time += elapsed;

        double avg_wait = total_tasks_processed ? total_wait_time / total_tasks_processed : 0;
        double throughput = elapsed > 0 ? total_tasks_processed / elapsed : 0;

        printf("Tâche PID %d terminée\n", task.pid);
        printf("Tâches traitées : %d\n", total_tasks_processed);
        printf("Temps d'attente moyen : %.2f s\n", avg_wait);
        printf("Débit (throughput) : %.2f tâches/s\n\n", throughput);

        // Mise à jour JSON après traitement
        update_json(fifo, task.pid, total_tasks_processed, avg_wait, throughput);
    }
}

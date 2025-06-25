#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "../common/fifo.h"
#include "ordonnanceur_fifo.h"

void ordonnanceur_fifo(fifo_t *fifo) {
    int total_tasks_processed = 0;
    double total_wait_time = 0.0;
    clock_t start_time = clock();

    while (1) {
        // Attente active si file vide (tu peux ajouter sleep ou condition selon ta logique)
        if (fifo_empty(fifo)) {
            printf("🕒 File vide, ordonnanceur en attente...\n");
            sleep(1);
            continue;
        }

        // Récupérer la tâche
        process_t task;
        fifo_dequeue(fifo, &task);

        // Simuler traitement
        printf("Traitement tâche PID=%d\n", task.pid);
        total_tasks_processed++;

        sleep(1); // simulation de traitement

        clock_t now = clock();
        double elapsed = (double)(now - start_time) / CLOCKS_PER_SEC;
        total_wait_time += elapsed;

        double avg_wait = total_tasks_processed ? total_wait_time / total_tasks_processed : 0;
        double throughput = elapsed > 0 ? total_tasks_processed / elapsed : 0;

        printf("Tâche PID %d terminée\n", task.pid);
        printf("Tâches traitées : %d\n", total_tasks_processed);
        printf("Temps d'attente moyen : %.2f s\n", avg_wait);
        printf("Débit (throughput) : %.2f tâches/s\n\n", throughput);

        // Mise à jour JSON (optionnelle)
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
    }
}

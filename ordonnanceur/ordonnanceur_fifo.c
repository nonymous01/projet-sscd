#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "../common/fifo.h"

// Chemin de la mémoire partagée
#define SHM_KEY 0x1234

// Fonction pour initialiser le fichier JSON
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

int main() {
    printf("Ordonnanceur FIFO\n");

    // Créer le fichier JSON dès le lancement
    init_stats_json();

    // Récupérer l'identifiant de la mémoire partagée
    int shmid = shmget(SHM_KEY, sizeof(fifo_t), 0666);
    if (shmid < 0) {
        perror("shmget ordonnanceur");
        exit(1);
    }

    // Attacher la mémoire partagée
    fifo_t *fifo = (fifo_t *)shmat(shmid, NULL, 0);
    if (fifo == (void *)-1) {
        perror("shmat ordonnanceur");
        exit(1);
    }

    // Ne PAS ré-initialiser la FIFO ici !

    int total_tasks_processed = 0;
    double total_wait_time = 0.0;
    clock_t start_time = clock();

    while (1) {
        process_t task;
        int ret = fifo_dequeue(fifo, &task);
        if (ret == 0) {
            printf("Traitement tâche PID=%d\n", task.pid);
            total_tasks_processed++;

            sleep(1);  // Simuler le traitement

            clock_t now = clock();
            double elapsed = (double)(now - start_time) / CLOCKS_PER_SEC;
            total_wait_time += elapsed;

            double avg_wait = total_tasks_processed ? total_wait_time / total_tasks_processed : 0;
            double throughput = elapsed > 0 ? total_tasks_processed / elapsed : 0;

            printf("Tâche PID %d terminée\n", task.pid);
            printf("Tâches traitées : %d\n", total_tasks_processed);
            printf("Temps d'attente moyen : %.2f s\n", avg_wait);
            printf("Débit (throughput) : %.2f tâches/s\n\n", throughput);

            // Mettre à jour le fichier JSON
            FILE *fp = fopen("ordonnanceur_output.json", "w");
            if (fp) {
                fprintf(fp, "{\n");
                fprintf(fp, "  \"tasks_in_queue\": [");  // Affichage simplifié
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
            printf("Ordonnanceur FIFO : pas de tâches, attente...\n");
            sleep(1);
        }
    }

    // Détacher la mémoire partagée avant de quitter (jamais atteint ici)
    shmdt(fifo);
    return 0;
}

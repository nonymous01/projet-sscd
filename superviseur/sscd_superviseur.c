#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#include "../common/fifo.h"
#include "../ordonnanceur/ordonnanceur_fifo.h"

#define PORT 8080
#define MAX_CLIENTS 10
#define SHM_KEY 0x1234

int server_fd;
int shmid;
fifo_t *fifo = NULL;  // pointeur global FIFO partagée

void log_message(const char *msg) {
    time_t now = time(NULL);
    char timestr[26];
    ctime_r(&now, timestr);
    timestr[strlen(timestr) - 1] = '\0';
    printf("[%s] %s\n", timestr, msg);
}

void handle_shutdown(int sig) {
    log_message("Signal d'arrêt reçu, fermeture du serveur...");
    if (server_fd > 0) close(server_fd);
    // Détacher la mémoire partagée
    if (fifo != NULL) shmdt(fifo);
    // Supprimer la mémoire partagée
    shmctl(shmid, IPC_RMID, NULL);
    exit(0);
}

void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getpeername(client_sock, (struct sockaddr *)&addr, &len);
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ipstr, sizeof(ipstr));
    printf("Connexion client depuis %s:%d\n", ipstr, ntohs(addr.sin_port));

    char buffer[1024] = {0};
    ssize_t received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (received > 0) {
        buffer[received] = '\0';
        printf("Message reçu du client : %s\n", buffer);

        if (fifo != NULL) {
            int val = atoi(buffer);

            process_t proc;
            proc.pid = val;
            proc.burst_time = 10;
            proc.remaining_time = 10;
            proc.state = READY;
            proc.start_time = 0;
            proc.end_time = 0;
            strcpy(proc.user, "unknown");
            strcpy(proc.command, "commande");
            proc.priority = 0;
            proc.cpu_usage = 0.0;
            proc.mem_usage = 0.0;

            if (fifo_enqueue(fifo, proc) == 0) {
                printf("Tâche PID %d ajoutée à la file FIFO\n", val);

                // === Mise à jour immédiate du fichier ordonnanceur_output.json ===
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
                    fprintf(fp, "  \"current_task\": null,\n");
                    fprintf(fp, "  \"tasks_processed\": 0,\n");
                    fprintf(fp, "  \"average_wait_time\": 0.00,\n");
                    fprintf(fp, "  \"throughput\": 0.00\n");
                    fprintf(fp, "}\n");
                    fclose(fp);
                }
                // === Fin mise à jour JSON ===
            } else {
                printf("⚠️ File FIFO pleine, tâche rejetée\n");
            }
        } else {
            fprintf(stderr, "FIFO non attachée dans handle_client\n");
        }
    }

    send(client_sock, "ACK\n", 4, 0);
    close(client_sock);
    pthread_exit(NULL);
}


// Thread ordonnanceur
void *thread_ordonnanceur(void *arg) {
    (void)arg; // non utilisé

    // Ici tu appelles ta fonction ordonnanceur qui tourne en boucle
    // Exemple hypothétique : ordonnanceur_fifo(fifo);

    // Si ordonnanceur_fifo() ne prend pas d'argument, adapte le code
    // Par exemple, tu peux modifier ordonnanceur_fifo() pour prendre fifo_t* en paramètre

    ordonnanceur_fifo(fifo);  // <- à adapter selon ta fonction ordonnanceur

    pthread_exit(NULL);
}

int main() {
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    signal(SIGTERM, handle_shutdown);
    signal(SIGINT, handle_shutdown);

    // Création mémoire partagée
    shmid = shmget(SHM_KEY, sizeof(fifo_t), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget superviseur");
        exit(1);
    }

    fifo = (fifo_t *)shmat(shmid, NULL, 0);
    if (fifo == (void *)-1) {
        perror("shmat superviseur");
        exit(1);
    }

    fifo_init(fifo);  // Initialisation FIFO
    log_message("✅ FIFO initialisée dans la mémoire partagée");

    // Lancement thread ordonnanceur
    pthread_t ord_thread;
    if (pthread_create(&ord_thread, NULL, thread_ordonnanceur, NULL) != 0) {
        perror("pthread_create ordonnanceur");
        exit(1);
    }
    pthread_detach(ord_thread);

    // Socket serveur
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    log_message("Superviseur démarré sur le port 8080");

    while (1) {
        int client_sock = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (client_sock < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        pthread_t thread_id;
        int *pclient = malloc(sizeof(int));
        *pclient = client_sock;
        pthread_create(&thread_id, NULL, handle_client, pclient);
        pthread_detach(thread_id);
    }

    // Ne sera jamais atteint normalement
    shmdt(fifo);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}

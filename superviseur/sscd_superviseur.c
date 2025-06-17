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

void log_message(const char *msg) {
    time_t now = time(NULL);
    char timestr[26];
    ctime_r(&now, timestr);
    timestr[strlen(timestr) - 1] = '\0';
    printf("[%s] %s\n", timestr, msg);
}

void handle_shutdown(int sig) {
    log_message("Signal d'arrêt reçu, fermeture du serveur...");
    close(server_fd);
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

        // Ajout dans la mémoire partagée
        int shmid = shmget(SHM_KEY, sizeof(fifo_t), 0666);
        if (shmid < 0) {
            perror("shmget client");
        } else {
            fifo_t *fifo = (fifo_t *)shmat(shmid, NULL, 0);
            if (fifo != (void *)-1) {
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
                } else {
                    printf("⚠️ File FIFO pleine, tâche rejetée\n");
                }

                shmdt(fifo);
            }
        }
    }

    send(client_sock, "ACK\n", 4, 0);
    close(client_sock);
    pthread_exit(NULL);
}

int main() {
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    signal(SIGTERM, handle_shutdown);
    signal(SIGINT, handle_shutdown);

    // Création mémoire partagée
    int shmid = shmget(SHM_KEY, sizeof(fifo_t), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget superviseur");
        exit(1);
    }

    fifo_t *fifo = (fifo_t *)shmat(shmid, NULL, 0);
    fifo_init(fifo);  // ✅ Initialisation correcte
    shmdt(fifo);
    log_message("✅ FIFO initialisée dans la mémoire partagée");

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

    return 0;
}

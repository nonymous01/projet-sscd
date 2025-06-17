#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "../common/fifo.h"
#include "../ordonnanceur/ordonnanceur_fifo.h"

#define MAX_PROCS 1024
#define SHM_KEY 0x1234

typedef struct {
    int pid;
    char user[32];
    char cmd[256];
    char state;
    float cpu_percent;
    float mem_percent;
} ProcessInfo;

// Fonction utilitaire pour vérifier si une chaîne est numérique
int is_number(const char *s) {
    for (int i = 0; s[i]; i++) {
        if (!isdigit(s[i])) return 0;
    }
    return 1;
}

unsigned long long get_total_cpu_time() {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    fclose(fp);
    return user + nice + system + idle + iowait + irq + softirq + steal;
}

unsigned long long get_process_time(int pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;

    unsigned long long utime = 0, stime = 0;
    char buf[1024];
    fgets(buf, sizeof(buf), fp);
    char *token = strtok(buf, " ");
    for (int i = 1; i <= 15 && token; i++) {
        token = strtok(NULL, " ");
        if (i == 13) utime = strtoull(token, NULL, 10);
        if (i == 14) stime = strtoull(token, NULL, 10);
    }
    fclose(fp);
    return utime + stime;
}

void write_json_output(ProcessInfo *procs, int count) {
    FILE *fp = fopen("moniteur/output.json", "w");
    if (!fp) {
        perror("fopen output.json");
        return;
    }

    fprintf(fp, "[\n");
    for (int i = 0; i < count; i++) {
        fprintf(fp,
            "  {\n"
            "    \"pid\": %d,\n"
            "    \"user\": \"%s\",\n"
            "    \"cmd\": \"%s\",\n"
            "    \"state\": \"%c\",\n"
            "    \"cpu_percent\": %.2f,\n"
            "    \"mem_percent\": %.2f\n"
            "  }%s\n",
            procs[i].pid, procs[i].user, procs[i].cmd, procs[i].state,
            procs[i].cpu_percent, procs[i].mem_percent,
            i < count - 1 ? "," : ""
        );
    }
    fprintf(fp, "]\n");

    fclose(fp);
}

// Ta fonction moniteur classique encapsulée
void run_moniteur_classique() {
    DIR *dir = opendir("/proc");
    if (!dir) {
        perror("opendir /proc");
        exit(1);
    }

    struct dirent *entry;
    ProcessInfo procs[MAX_PROCS];
    int count = 0;

    unsigned long long total_cpu_before = get_total_cpu_time();
    unsigned long long proc_times_before[MAX_PROCS] = {0};
    int pids[MAX_PROCS] = {0};

    // Première lecture pour CPU
    while ((entry = readdir(dir)) && count < MAX_PROCS) {
        if (!is_number(entry->d_name)) continue;
        int pid = atoi(entry->d_name);
        pids[count] = pid;
        proc_times_before[count] = get_process_time(pid);
        count++;
    }
    closedir(dir);
    sleep(1); // Attente pour calculer delta CPU
    unsigned long long total_cpu_after = get_total_cpu_time();

    count = 0;
    dir = opendir("/proc");
    if (!dir) {
        perror("opendir /proc");
        exit(1);
    }
    while ((entry = readdir(dir)) && count < MAX_PROCS) {
        if (!is_number(entry->d_name)) continue;
        int pid = atoi(entry->d_name);

        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/stat", pid);
        FILE *fp = fopen(path, "r");
        if (!fp) continue;

        char comm[256], state;
        fscanf(fp, "%*d (%[^)]) %c", comm, &state);
        fclose(fp);

        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        fp = fopen(path, "r");
        if (!fp) continue;

        int uid = -1;
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "Uid:\t%d", &uid) == 1) break;
        }
        fclose(fp);

        struct passwd *pw = getpwuid(uid);
        char *user = pw ? pw->pw_name : "unknown";

        unsigned long long proc_time_after = get_process_time(pid);
        int index = -1;
        for (int i = 0; i < MAX_PROCS; i++) {
            if (pids[i] == pid) {
                index = i;
                break;
            }
        }

        float cpu_percent = 0;
        if (index >= 0 && total_cpu_after > total_cpu_before) {
            cpu_percent = 100.0 * (proc_time_after - proc_times_before[index]) / (total_cpu_after - total_cpu_before);
        }

        snprintf(path, sizeof(path), "/proc/%d/statm", pid);
        fp = fopen(path, "r");
        if (!fp) continue;

        unsigned long mem_pages;
        fscanf(fp, "%lu", &mem_pages);
        fclose(fp);

        long page_size = sysconf(_SC_PAGESIZE);
        long mem_kb = mem_pages * page_size / 1024;
        long total_mem_kb = sysconf(_SC_PHYS_PAGES) * page_size / 1024;
        float mem_percent = 100.0 * mem_kb / total_mem_kb;

        procs[count].pid = pid;
        strncpy(procs[count].user, user, sizeof(procs[count].user));
        strncpy(procs[count].cmd, comm, sizeof(procs[count].cmd));
        procs[count].state = state;
        procs[count].cpu_percent = cpu_percent;
        procs[count].mem_percent = mem_percent;

        count++;
    }
    closedir(dir);

    write_json_output(procs, count);
    printf("JSON généré avec %d processus.\n", count);
}

// Affiche la FIFO partagée
void afficher_fifo(const fifo_t *fifo) {
    printf("FIFO contient %d processus:\n", fifo->count);
    int i = fifo->front;
    for (int c = 0; c < fifo->count; c++) {
        process_t p = fifo->tasks[i];
        printf("PID=%d, State=%d, Burst=%d\n", p.pid, p.state, p.burst_time);
        i = (i + 1) % MAX_TASKS;
    }
}

volatile int running = 1;
void handle_sigint(int sig) { running = 0; }

int main(int argc, char *argv[]) {
    int show_fifo = 0;
    int run_scheduler = 0;
    int generate_json = 0;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fifo") == 0) show_fifo = 1;
        else if (strcmp(argv[i], "--run-scheduler") == 0) run_scheduler = 1;
        else if (strcmp(argv[i], "--json") == 0) generate_json = 1;
    }

    signal(SIGINT, handle_sigint);

    fifo_t *fifo = NULL;
    int shmid = -1;
    if (show_fifo || run_scheduler) {
        shmid = shmget(SHM_KEY, sizeof(fifo_t), 0666);
        if (shmid < 0) {
            perror("shmget");
            return 1;
        }
        fifo = (fifo_t *)shmat(shmid, NULL, 0);
        if (fifo == (void *)-1) {
            perror("shmat");
            return 1;
        }
    }

    if (!show_fifo && !run_scheduler && !generate_json) {
        // Pas d'option : moniteur classique une seule fois puis exit
        run_moniteur_classique();
        if (fifo != NULL) shmdt(fifo);
        return 0;
    }

    while (running) {
        system("clear");
        printf("=== Moniteur Système ===\n");

        if (show_fifo && fifo != NULL) {
            printf("\n--- Affichage FIFO ---\n");
            afficher_fifo(fifo);
        }

        if (run_scheduler && fifo != NULL) {
            printf("\n--- Exécution ordonnanceur FIFO ---\n");
            ordonnanceur_fifo(fifo);
        }

        if (generate_json) {
            run_moniteur_classique(); // génère le json (affiche un message)
        }

        sleep(2);
    }

    if (fifo != NULL) shmdt(fifo);

    printf("\nArrêt du moniteur.\n");
    return 0;
}

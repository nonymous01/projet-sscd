# Projet SSCD – Superviseur, Ordonnanceur FIFO, Moniteur Système

Ce projet simule un système de supervision et d’ordonnancement de tâches à l’aide de la mémoire partagée et des sockets TCP. Il comprend :

- Un superviseur (`sscd_superviseur`) pour recevoir les tâches et les ajouter dans une file partagée.
- Un ordonnanceur FIFO (`ordonnanceur_fifo`) qui lit et traite les tâches en mettant à jour un fichier JSON.
- Un moniteur (`moniteur_systeme`) pour visualiser les informations système.
- Un client (`ajout_tache`) permettant d’envoyer une tâche au superviseur.

##  Prérequis

- Linux (Debian/Mint/Ubuntu recommandé)
- `gcc`, `make`
- Support des threads (`pthread`) et mémoire partagée (`sysv shm`)

##  Compilation

Dans le dossier du projet :

```bash
make clean && make
```

Les exécutables suivants seront générés :

- `sscd_superviseur`
- `ordonnanceur_fifo`
- `moniteur_systeme`
- `ajout_tache`

##  Lancement (3 terminaux recommandés)

### Terminal 1 – Superviseur

```bash
./sscd_superviseur
```

Lance le serveur qui reçoit les tâches via le port TCP `8080`.

### Terminal 2 – Ordonnanceur FIFO

```bash
./ordonnanceur_fifo
```

Lit les tâches dans la mémoire partagée, les traite et met à jour `ordonnanceur_output.json`.

### Terminal 3 – Ajout d'une tâche

```bash
./ajout_tache <PID>
```

Exemple :

```bash
./ajout_tache 123
```

Cela envoie une tâche PID=123 au superviseur.

### (Optionnel) Terminal 4 – Moniteur Système

```bash
./moniteur_systeme
```

Affiche des informations sur les tâches et le système.

##  Fichier de sortie

- `ordonnanceur_output.json` : généré automatiquement, contient les statistiques du système (tâches en file, tâche en cours, temps d’attente, etc.).

##  Nettoyage

Pour supprimer les fichiers générés :

```bash
make clean
```

##  Structure du projet

```
projet-sscd/
├── common/                  # FIFO générique partagée
├── ordonnanceur/           # Ordonnanceur FIFO
├── superviseur/            # Superviseur serveur
├── moniteur/               # Moniteur système
├── tests/                  # Client ajout_tache
├── ordonnanceur_output.json
├── Makefile
└── README.md
```

---
By Alien 👽
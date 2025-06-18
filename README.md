# Projet SSCD – Superviseur, Ordonnanceur FIFO, Moniteur Système

Ce projet simule un système de supervision et d’ordonnancement de tâches à l’aide de la mémoire partagée et des sockets TCP. Il comprend :

- Un superviseur (`sscd_superviseur`) pour recevoir les tâches et les ajouter dans une file partagée.
- Un ordonnanceur FIFO (`ordonnanceur_fifo`) qui lit et traite les tâches en mettant à jour un fichier JSON.
- Un moniteur (`moniteur_systeme`) pour visualiser les informations système.
- Un client (`ajout_tache`) permettant d’envoyer une tâche au superviseur.
- Un dashboard web (`dashboard.py`) pour visualiser les données du système.

##  Prérequis

- Linux (Debian/Mint/Ubuntu recommandé)
- `gcc`, `make`
- Python 3 avec `dash`, `pandas`, `flask`
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

##  Lancement (4 terminaux recommandés)

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

### Terminal 4 – Dashboard Web (interface graphique)

Active l’environnement Python virtuel puis exécute le dashboard :

```bash
source env/bin/activate
python dashboard.py
```

Sortie attendue :

```
Dash is running on http://127.0.0.1:8050/
 * Serving Flask app 'dashboard'
 * Debug mode: on
```

Ouvre ensuite ton navigateur à [http://127.0.0.1:8050](http://127.0.0.1:8050) pour visualiser les tâches et les statistiques.

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
├── dashboard.py            # Dashboard web Dash
├── ordonnanceur_output.json
├── Makefile
└── README.md
```

---

By Alien 👽
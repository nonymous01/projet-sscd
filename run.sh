#!/bin/bash

echo " [SSCD] Démarrage complet du système..."

# === 1. Active l’environnement virtuel Python
if [ -d "env" ]; then
    source env/bin/activate
    echo " Environnement virtuel activé"
else
    echo " Environnement virtuel introuvable. Veuillez créer 'env/' avec 'python3 -m venv env'"
    exit 1
fi

# === 2. Compilation des modules C
echo " Compilation des modules C..."
make clean && make

if [ $? -ne 0 ]; then
    echo " Erreur de compilation. Arrêt."
    exit 1
fi
echo " Compilation terminée."

# === 3. Lancement des composants C
echo " Lancement du moniteur système..."
./moniteur_systeme &

echo " Lancement de l’ordonnanceur FIFO..."
./sscd_superviseur &

# Attendre un peu pour que le superviseur écoute
sleep 1

echo " Injection automatique de tâches..."
# Envoie des tâches en boucle (simulation)
for pid in 100 101 102 103 104 105 106; do
    ./ajout_tache $pid
    sleep 0.2
done

# === 4. Lancement du dashboard Python
echo " Lancement du dashboard Python..."
xdg-open http://127.0.0.1:8050 > /dev/null 2>&1 &
python3 dashboard.py

CC = gcc
CFLAGS = -Wall -pthread

# Superviseur
SUPERVISEUR_SRC = superviseur/sscd_superviseur.c
SUPERVISEUR_BIN = sscd_superviseur

# Modules utilisés
ORDO_OBJ = ordonnanceur/ordonnanceur_fifo.o
FIFO_OBJ = common/fifo.o

# Ajout Tâche
TESTS_SRC = tests/ajout_tache.c
TESTS_BIN = ajout_tache

# Moniteur Système
MONITEUR_SRC = moniteur/moniteur_systeme.c
MONITEUR_BIN = moniteur_systeme

# Cible principale
all: $(SUPERVISEUR_BIN) $(TESTS_BIN) $(MONITEUR_BIN)

# Superviseur : compilation avec modules ordonnanceur et fifo
$(SUPERVISEUR_BIN): $(SUPERVISEUR_SRC) $(ORDO_OBJ) $(FIFO_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation des modules en objets
$(ORDO_OBJ): ordonnanceur/ordonnanceur_fifo.c ordonnanceur/ordonnanceur_fifo.h
	$(CC) $(CFLAGS) -c $< -o $@

$(FIFO_OBJ): common/fifo.c common/fifo.h
	$(CC) $(CFLAGS) -c $< -o $@

# Ajout de tâche (indépendant, mais lié à FIFO)
$(TESTS_BIN): $(TESTS_SRC) $(FIFO_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Moniteur système (ajoute ordonnanceur_fifo.o et fifo.o pour linker)
$(MONITEUR_BIN): $(MONITEUR_SRC) $(ORDO_OBJ) $(FIFO_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Nettoyage
clean:
	rm -f $(SUPERVISEUR_BIN) $(TESTS_BIN) $(MONITEUR_BIN) $(ORDO_OBJ) $(FIFO_OBJ)

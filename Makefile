CC = gcc
CFLAGS = -Wall -pthread

# Binaries
SUPERVISEUR_BIN = sscd_superviseur
TESTS_BIN = ajout_tache
MONITEUR_BIN = moniteur_systeme

# Sources
SUPERVISEUR_SRC = superviseur/sscd_superviseur.c
ORDO_LOGIC_SRC = ordonnanceur/ordonnanceur_logic.c
TESTS_SRC = tests/ajout_tache.c
MONITEUR_SRC = moniteur/moniteur_systeme.c
FIFO_SRC = common/fifo.c

# Objets
ORDO_LOGIC_OBJ = ordonnanceur/ordonnanceur_logic.o
FIFO_OBJ = common/fifo.o

# Compilation globale
all: $(SUPERVISEUR_BIN) $(TESTS_BIN) $(MONITEUR_BIN)

# Superviseur : utilise ordonnanceur_fifo() sans main
$(SUPERVISEUR_BIN): $(SUPERVISEUR_SRC) $(ORDO_LOGIC_OBJ) $(FIFO_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Moniteur : aussi dépendant de ordonnanceur_fifo()
$(MONITEUR_BIN): $(MONITEUR_SRC) $(ORDO_LOGIC_OBJ) $(FIFO_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Test indépendant (ajout_tache) — pas besoin de ordonnanceur
$(TESTS_BIN): $(TESTS_SRC) $(FIFO_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Objets
$(ORDO_LOGIC_OBJ): ordonnanceur/ordonnanceur_logic.c ordonnanceur/ordonnanceur_fifo.h
	$(CC) $(CFLAGS) -c $< -o $@

$(FIFO_OBJ): $(FIFO_SRC) common/fifo.h
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -f $(SUPERVISEUR_BIN) $(TESTS_BIN) $(MONITEUR_BIN) \
	      $(ORDO_LOGIC_OBJ) $(FIFO_OBJ)

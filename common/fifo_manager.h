#ifndef FIFO_MANAGER_H
#define FIFO_MANAGER_H

#include "fifo.h"

fifo_t* attacher_fifo(void);
int ajouter_tache(fifo_t *fifo, int id_tache);

#endif

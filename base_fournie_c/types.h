#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

typedef struct {
    int l;   /* ligne   */
    int c;   /* colonne */
} Pos;


#define DIRS "NSEO"


bool pos_egales(Pos a, Pos b);

Pos voisin(Pos p, char dir);

#endif
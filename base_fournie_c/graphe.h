#ifndef GRAPHE_H
#define GRAPHE_H

#include <stdbool.h>
#include "types.h"
#include "carte_robot.h"

#define PAS_DE_CHEMIN 'X'

bool est_a_cote(Pos a, Pos b);
char prochain_pas(const CARTE_ROBOT *carte, Pos depart, Pos objet);

#endif
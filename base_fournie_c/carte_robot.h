#ifndef CARTE_ROBOT_H
#define CARTE_ROBOT_H

#include "types.h"
#include "interface.h"
#include <stdbool.h>

#define HAUTEUR_MAX 32
#define LARGEUR_MAX 32

typedef enum { INCONNUE, LIBRE, OCCUPEE } TypeCase;

typedef struct {
    TypeCase cases[HAUTEUR_MAX][LARGEUR_MAX];
    int hauteur, largeur;
} CARTE_ROBOT;

void init_carte_robot( CARTE_ROBOT *carte, int hauteur, int largeur);/* initialise la carte tout est inconnue */
void mettre_a_jour_carte_robot( CARTE_ROBOT *carte, Pos pos, TypeCase type); /* met à jour la case de la carte */
bool est_dans_carte(const CARTE_ROBOT *carte, Pos pos); /* la case existe ? */
bool case_valide(const CARTE_ROBOT *carte, Pos pos); /* le robot peut passer ? (INCONNUE ou LIBRE) */

#endif
#include "graphe.h"
#include <stdlib.h>

bool est_a_cote(Pos a, Pos b) {
    return (abs(a.l - b.l) + abs(a.c - b.c)) == 1;
}

//on fait un bfs pour trouver le prochain pas vers lobjet
char prochain_pas(const CARTE_ROBOT *carte, Pos depart, Pos objet) {
    char premier_pas[HAUTEUR_MAX][LARGEUR_MAX] = {0}; 
    Pos file[HAUTEUR_MAX * LARGEUR_MAX];             
    int debut = 0;
    int fin = 0;
    for (int i = 0; i < 4; i++) {
        Pos v = voisin(depart, DIRS[i]);
        if (case_valide(carte, v)) {
            premier_pas[v.l][v.c] = DIRS[i];
            file[fin] = v;
            fin++;
        }
    }
    while (debut < fin) {
        Pos courant = file[debut];
        debut++;
        if (est_a_cote(courant, objet)) {
            return premier_pas[courant.l][courant.c];
        }

        for (int i = 0; i < 4; i++) {
            Pos v = voisin(courant, DIRS[i]);
            if (case_valide(carte, v) && premier_pas[v.l][v.c] == 0 && !pos_egales(v, depart)) {
                premier_pas[v.l][v.c] = premier_pas[courant.l][courant.c];
                file[fin] = v;
                fin++;
            }
        }
    }
    return PAS_DE_CHEMIN;
}
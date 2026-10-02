#include "graphe.hpp"
#include <cstdlib>
#include <vector>

bool est_a_cote(Pos a, Pos b) {
    return (std::abs(a.l - b.l) + std::abs(a.c - b.c)) == 1;
}

//on fait un bfs pour trouver le prochain pas vers lobjet
char prochain_pas(const CarteRobot &carte, Pos depart, Pos objet) {
    std::vector<std::vector<char>> premier_pas(carte.hauteur(), std::vector<char>(carte.largeur(), 0));
    std::vector<Pos> file(carte.hauteur() * carte.largeur());
    int debut = 0;
    int fin = 0;
    for (int i = 0; i < 4; i++) {
        Pos v = voisin(depart, DIRS[i]);
        if (carte.case_valide(v)) {
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
            if (carte.case_valide(v) && premier_pas[v.l][v.c] == 0 && !pos_egales(v, depart)) {
                premier_pas[v.l][v.c] = premier_pas[courant.l][courant.c];
                file[fin] = v;
                fin++;
            }
        }
    }
    return PAS_DE_CHEMIN;
}

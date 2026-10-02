#include "interface.hpp"

#include <cstdio>
#include "graphe.hpp"

int main()
{
    /* La mini-grille de l'énoncé (B.3), avec l'armoire A en (1,4) :
       ######
       #R.#A#
       #.#..#
       #...##
       ######        */
    CarteRobot carte(5, 6);
    carte.mettre_a_jour(Pos{1, 3}, TypeCase::OCCUPEE);   /* mur */
    carte.mettre_a_jour(Pos{1, 4}, TypeCase::OCCUPEE);   /* armoire */
    carte.mettre_a_jour(Pos{2, 2}, TypeCase::OCCUPEE);   /* mur */
    carte.mettre_a_jour(Pos{3, 4}, TypeCase::OCCUPEE);   /* mur */

    Pos robot = {1, 1};
    Pos armoire = {1, 4};
    int nb_pas = 0;

    /* Le robot avance pas à pas jusqu'à être à côté de l'armoire */
    while (!est_a_cote(robot, armoire)) {
        char d = prochain_pas(carte, robot, armoire);
        if (d == PAS_DE_CHEMIN) {
            std::printf("aucun chemin\n");
            return 0;
        }
        std::printf("%c ", d);
        robot = voisin(robot, d);
        nb_pas++;
    }
    std::printf("\n%d pas, arrive en (%d,%d)\n", nb_pas, robot.l, robot.c);

    /* On mure le passage (3,2) : plus aucun chemin */
    carte.mettre_a_jour(Pos{3, 2}, TypeCase::OCCUPEE);
    std::printf("%c\n", prochain_pas(carte, Pos{1, 1}, armoire));
    return 0;
}

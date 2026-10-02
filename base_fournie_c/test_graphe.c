#include <stdio.h>
#include "graphe.h"

int main(void)
{
    /* La mini-grille de l'énoncé (B.3), avec l'armoire A en (1,4) :
       ######
       #R.#A#
       #.#..#
       #...##
       ######        */
    CARTE_ROBOT carte;
    init_carte_robot(&carte, 5, 6);
    mettre_a_jour_carte_robot(&carte, (Pos){1, 3}, OCCUPEE);   /* mur */
    mettre_a_jour_carte_robot(&carte, (Pos){1, 4}, OCCUPEE);   /* armoire */
    mettre_a_jour_carte_robot(&carte, (Pos){2, 2}, OCCUPEE);   /* mur */
    mettre_a_jour_carte_robot(&carte, (Pos){3, 4}, OCCUPEE);   /* mur */

    Pos robot = {1, 1};
    Pos armoire = {1, 4};
    int nb_pas = 0;

    /* Le robot avance pas à pas jusqu'à être à côté de l'armoire */
    while (!est_a_cote(robot, armoire)) {
        char d = prochain_pas(&carte, robot, armoire);
        if (d == PAS_DE_CHEMIN) {
            printf("aucun chemin\n");
            return 0;
        }
        printf("%c ", d);
        robot = voisin(robot, d);
        nb_pas++;
    }
    printf("\n%d pas, arrive en (%d,%d)\n", nb_pas, robot.l, robot.c);

    /* On mure le passage (3,2) : plus aucun chemin */
    mettre_a_jour_carte_robot(&carte, (Pos){3, 2}, OCCUPEE);
    printf("%c\n", prochain_pas(&carte, (Pos){1, 1}, armoire));
    return 0;
}
#include "carte_robot.h" 

void init_carte_robot( CARTE_ROBOT *carte, int hauteur, int largeur){
    carte->hauteur = hauteur;
    carte->largeur = largeur;
    for (int i = 0; i < hauteur; i++) {
        for (int j = 0; j < largeur; j++) {
            if (i==0 || i==hauteur-1 || j==0 || j==largeur-1) {
                carte->cases[i][j] = OCCUPEE; /*on fait les bords*/
            } else {
                carte->cases[i][j] = INCONNUE; /*le reste est inconnu par le robot*/
            }
        }
    }

}

bool est_dans_carte(const CARTE_ROBOT *carte, Pos pos)
{
    if (pos.l >= 0 && pos.l < carte->hauteur && pos.c >= 0 && pos.c < carte->largeur) {
        return true;
    } else {
        return false;
    }
}

bool case_valide(const CARTE_ROBOT *carte, Pos pos)
{
    if (est_dans_carte(carte, pos) && carte->cases[pos.l][pos.c] != OCCUPEE) {
        return true;
    } else {
        return false;
    }
}

void mettre_a_jour_carte_robot(CARTE_ROBOT *carte, Pos pos, TypeCase type)
{
    if (est_dans_carte(carte, pos)) {
        carte->cases[pos.l][pos.c] = type;
    }
}

#ifndef CARTE_ROBOT_HPP
#define CARTE_ROBOT_HPP

#include <vector>
#include "types.hpp"

enum class TypeCase { INCONNUE, LIBRE, OCCUPEE };

class CarteRobot {
public:
    CarteRobot(int hauteur, int largeur);           /* initialise la carte tout est inconnue */
    void mettre_a_jour(Pos pos, TypeCase type);     /* met à jour la case de la carte */
    bool est_dans_carte(Pos pos) const;             /* la case existe ? */
    bool case_valide(Pos pos) const;                /* le robot peut passer ? (INCONNUE ou LIBRE) */
    int hauteur() const { return hauteur_; }
    int largeur() const { return largeur_; }

private:
    int hauteur_, largeur_;
    std::vector<std::vector<TypeCase>> cases_;
};

#endif

#ifndef MEMOIRE_HPP
#define MEMOIRE_HPP

#include <string>
#include "interface.hpp"
#include "carte_robot.hpp"

/* La MÉMOIRE du robot (énoncé, section 7) : tout ce qu'il sait,
   mis à jour à chaque perception. */
struct Memoire {
    CarteRobot carte;            /* ce qu'il sait de l'appartement */
    Pos robot;                   /* où il est */
    Pos selecteur;               /* où est le sélecteur de l'armoire */
    bool vu[3][8];               /* casiers déjà vus pendant la demande en cours */
    std::string contenu[3][8];   /* leur contenu ("" = vide) */

    Memoire(const Connaissances &k);
    void noter_perception(const PerceptionRobot &p);
    void oublier_armoire();
};

#endif
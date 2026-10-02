#include "interface.hpp"
#include <optional>

struct Environnement::Etat {
    std::vector<std::string> grille; // grille[i][j] = case (i, j) de l'environnement
    Pos pos_robot; // position du robot dans la grille
    Pos pos_selecteur; // position du selecteur dans l'armoire
    Pos pos_armoire; // position de l'armoire dans la grille
    Pos pos_dictionnaire; // position du dictionnaire dans la grille
    std::vector<ResidentConnu> residents; // liste de résidents de la carte (id et position)
    std::vector<std::vector<std::optional<std::string>>> armoire; // armoire[i][j] = case (i, j) de l'armoire
    std::optional<std::string> objet_porte;// objet actuellement sélectionné par le robot (vide si aucun)
    std::optional<Pos> resident_vise; // position du résident actuellement visé par le robot (vide si aucun)
    Pos depart; // position de départ du robot (case vide)
    Pos selecteur_depart; // position de départ du selecteur (case vide)

};

Environnement::Etat::~Etat() = default;
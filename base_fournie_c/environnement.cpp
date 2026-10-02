#include "interface.hpp"
#include <memory>
#include <optional>

struct Environnement::Etat {
    std::vector<std::string> grille; // grille[i][j] = case (i, j) de l'environnement
    Pos pos_robot; // position du robot dans la grille
    Pos pos_selecteur; // position du selecteur dans l'armoire
    Pos pos_armoire; // position de l'armoire dans la grille
    Pos pos_dictionnaire; // position du dictionnaire dans la grille
    std::vector<ResidentConnu> residents; // liste de résidents de la carte (id et position)
    std::vector<std::vector<std::optional<std::string>>> armoire; // armoire[i][j] = case (i, j) de l'armoire
    std::optional<std::string> objet_porte;// objet actuellement tenu par le robot (vide si aucun)
    std::optional<Pos> resident_vise; // position du résident actuellement visé par le robot (vide si aucun)
    Pos depart; // position de départ du robot (case vide)
    Pos selecteur_depart; // position de départ du selecteur (case vide)

};

Environnement::~Environnement() = default;

Environnement::Environnement(const JsonValue &carte, const JsonValue &armoire)
{   
    etat = std::make_unique<Etat>();
    const JsonValue &grille_json = *carte.get("grille");
    for (size_t i = 0; i < grille_json.taille(); i++)
    {
        const JsonValue &ligne = *grille_json.index(i);
        etat->grille.push_back(*ligne.chaine());
    }
    
}
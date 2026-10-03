#include "interface.hpp"
#include <memory>
#include <optional>

struct Environnement::Etat {
    std::vector<std::string> grille; // grille[i][j] = case (i, j) de l'environnement

    Pos pos_robot; // position du robot dans la grille
    Pos pos_selecteur; // position du selecteur dans l'armoire
    Pos pos_armoire; // position de l'armoire dans la grille
    Pos pos_dictionnaire; // position du dictionnaire dans la grille
    Pos depart_robot; // position de départ du robot (case vide)
    Pos casier_depart; // position de départ du selecteur (case vide)

    std::vector<ResidentConnu> residents; // liste de résidents de la carte (id et position)
    std::vector<std::vector<std::optional<std::string>>> armoire; // armoire[i][j] = case (i, j) de l'armoire
    std::optional<std::string> objet_porte;// objet actuellement tenu par le robot (vide si aucun)
    std::optional<Pos> resident_vise; // position du résident actuellement visé par le robot (vide si aucun)



};

Environnement::~Environnement() = default;

Environnement::Environnement(const JsonValue &carte, const JsonValue &armoire)
{   
    // Initialiser l'état de l'environnement
    etat = std::make_unique<Etat>();

    // Lire la grille depuis le JSON
    const JsonValue &grille_json = *carte.get("grille");

    // Lire la grille depuis le JSON et la stocker dans l'état
    for (size_t i = 0; i < grille_json.taille(); i++)
    {
        const JsonValue &ligne = *grille_json.index(i);
        etat->grille.push_back(*ligne.chaine());
    }

    // Initialiser les positions courantes de départ du robot et du selecteur depuis le JSON
    carte.get("depart_robot")->paire_entiers(etat->depart_robot.l, etat->depart_robot.c);
    armoire.get("casier_depart")->paire_entiers(etat->casier_depart.l, etat->casier_depart.c);
    
    // Initialiser les positions courantes de l'armoire et du dictionnaire depuis le JSON
    carte.get("armoire")->get("position")->paire_entiers(etat->pos_armoire.l, etat->pos_armoire.c);
    carte.get("dictionnaire")->get("position")->paire_entiers(etat->pos_dictionnaire.l, etat->pos_dictionnaire.c);

    // Initialiser les positions courantes du robot et du selecteur au départ
    etat->pos_robot = etat->depart_robot;
    etat->pos_selecteur = etat->casier_depart;
    
    // Lire les résidents depuis le JSON et les stocker dans l'état
    const JsonValue &residents_json = *carte.get("residents");
    for (size_t i = 0; i < residents_json.taille(); i++)
    {
        const JsonValue &resident_json = *residents_json.index(i);
        ResidentConnu res;
        res.id = *resident_json.get("id")->chaine();
        resident_json.get("position")->paire_entiers(res.pos.l, res.pos.c);
        etat->residents.push_back(res);
        
    }

    // Lire l'armoire depuis le JSON et la stocker dans l'état
    size_t n_ligne = armoire.get("intensites")->taille(); // nombre de lignes de l'armoire
    size_t n_colonne = armoire.get("emotions")->taille(); // nombre de colonnes de l'armoire

    std::vector<std::optional<std::string>> ligne_armoire(n_colonne); // initialiser une ligne vide
    etat->armoire.assign(n_ligne, ligne_armoire); // initialiser l'armoire avec des lignes vides
    
    // Remplir l'armoire avec les objets depuis le JSON
    const JsonValue &casiers_json = *armoire.get("casiers");
    for (size_t i = 0; i < casiers_json.taille(); i++)
    {
        const JsonValue &casier = *casiers_json.index(i);
        int l = casier.get_nombre("ligne"); 
        int c = casier.get_nombre("colonne");
        const JsonValue &objet = *casier.get("objet");
        if (objet.est_chaine())
        {
            etat->armoire[l][c] = *objet.chaine(); // case avec un objet

        }
    }
}

    Pos Environnement::robot() const {
        return etat->pos_robot;
    }

    Pos Environnement::selecteur() const {
        return etat->pos_selecteur;
    }

    void Environnement::viser_resident(Pos resident) {
        etat->resident_vise = resident;
    }


Connaissances Environnement::connaissances() const{
        Connaissances k;
        k.hauteur = etat->grille.size();
        k.largeur = etat->grille.empty() ? 0 : etat->grille[0].size();
        k.depart = etat->depart_robot;
        k.armoire = etat->pos_armoire;
        k.dictionnaire = etat->pos_dictionnaire;
        k.residents = etat->residents;
        k.casier_depart = etat->casier_depart;
        return k;
}

std::string nom_case (char signe){
    switch (signe)
    {
    case '#' :
        return "mur";

    case '.' : 
        return "libre";
    
    case 'A' : 
        return "armoire";
    
    case 'D' : 
        return "dictionnaire";

    case 'P' : 
        return "resident";
        
    case 'R' : 
        return "libre";//la case de départ est une case libre

    default:
        return "mur";// on evite les cases inconnues
    }
}

PerceptionRobot Environnement::percevoir() const{
    char direction;
    PerceptionRobot p;

    for (size_t i = 0; i <=3; i++){
        direction = DIRS[i]; //NSEO
        Pos case_voisine = voisin(etat->pos_robot,direction); // donne la case voisine dans la direction DIRS[i]
        char char_grille = etat->grille[case_voisine.l][case_voisine.c]; //donne le char dans cette case voisine
        p.voisins[i] = nom_case(char_grille); // renvoie le type de la case voisine
        if (p.voisins[i].compare("armoire") == 0){
            p.devant_armoire = true;
        }
    }
    if (p.devant_armoire){
        p.contenu_casier = etat ->armoire[etat->pos_selecteur.l][etat->pos_selecteur.c];
    }

    return p;
} 


#include <cstdlib>
#include "choix_casier.hpp"

/* Cherche un mot dans une liste du dictionnaire et renvoie sa place (0, 1, 2...),
   ou -1 s'il n'y est pas. Sert aux deux fonctions juste en dessous. */
static int place_dans_liste(const JsonValue &dictionnaire, const std::string &liste, const std::string &mot)
{
    const JsonValue *tableau = dictionnaire.get(liste);
    if (tableau == nullptr) {
        return -1;
    }
    for (std::size_t k = 0; k < tableau->taille(); k++) {
        const std::string *s = tableau->index(k)->chaine();
        if (s != nullptr && *s == mot) {
            return (int)k;
        }
    }
    return -1;
}

/* "tristesse" -> 4 : l'ordre est celui du champ "emotions" du dictionnaire (énoncé 5.2). */
int numero_emotion(const JsonValue &dictionnaire, const std::string &emotion)
{
    return place_dans_liste(dictionnaire, "emotions", emotion);
}

/* "faible" -> 0, "moyenne" -> 1, "forte" -> 2 */
int numero_intensite(const JsonValue &dictionnaire, const std::string &intensite)
{
    return place_dans_liste(dictionnaire, "intensites", intensite);
}

/* L'étiquette "repli" écrite dans la trace (énoncé 6.2). */
std::string etiquette_repli(int distance, bool meme_intensite)
{
    if (distance == 0 && meme_intensite) return "aucun";
    if (distance == 0) return "intensite";
    if (distance == 1) return "voisine_1";
    return "voisine_2";
}

/* Les 15 casiers à essayer, déjà dans l'ordre de l'énoncé 6.2.
   emotion = numéro de 0 à 7, intensite = numéro de 0 à 2. */
std::vector<CasierPossible> casiers_a_essayer(int emotion, int intensite)
{
    std::vector<CasierPossible> liste;
    for (int distance = 0; distance <= 2; distance++) {                  /* 1. distance croissante */
        for (int ecart = 0; ecart <= 2; ecart++) {                       /* 2. écart d'intensité croissant */
            for (int autre_intensite = 0; autre_intensite <= 2; autre_intensite++) {   /* 3. faible d'abord */
                if (std::abs(intensite - autre_intensite) != ecart) continue;
                for (int sens : {+1, -1}) {                              /* 4. horaire puis antihoraire */
                    if (distance == 0 && sens == -1) continue;           /* à distance 0, une seule émotion */
                    CasierPossible casier;
                    casier.position = Pos{autre_intensite, roue_avancer(emotion, sens * distance, 8)};
                    casier.repli = etiquette_repli(distance, autre_intensite == intensite);
                    liste.push_back(casier);
                }
            }
        }
    }
    return liste;
}

/* Le prochain pas du sélecteur vers la cible : vertical d'abord (énoncé C.3).
   À n'appeler que si le sélecteur n'est pas déjà sur la cible. */
char pas_du_selecteur(Pos selecteur, Pos cible)
{
    if (cible.l > selecteur.l) return 'S';
    if (cible.l < selecteur.l) return 'N';
    int crans_vers_est = roue_avancer(cible.c, -selecteur.c, 8);   /* (c' - c) mod 8 */
    if (crans_vers_est <= 8 - crans_vers_est) return 'E';
    return 'O';
}

/* Le casier à traiter maintenant (énoncé 6.2) : le premier de la liste
   qui n'a pas encore été vu, ou qui a été vu garni.
   Renvoie son rang dans la liste (0 à 14), ou -1 si les 15 sont vus et vides. */
int prochain_casier(const std::vector<CasierPossible> &liste,
                    const bool vu[3][8], const std::string contenu[3][8])
{
    for (int rang = 0; rang < (int)liste.size(); rang++) {
        Pos p = liste[rang].position;
        if (!vu[p.l][p.c] || contenu[p.l][p.c] != "") {
            return rang;
        }
    }
    return -1;
}
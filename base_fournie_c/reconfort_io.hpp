/*
 * Base de code fournie -- projet "Robot de reconfort".
 *
 * Ce module fait DEUX choses, et rien d'autre :
 *
 *   1. lire les quatre fichiers d'entree (carte, dictionnaire, armoire,
 *      scenario) ;
 *   2. construire et exporter le fichier de trace attendu a la sortie.
 *
 * Tout le reste du projet -- perception, carte mentale, planification de
 * chemin, consultation du dictionnaire, fouille de l'armoire, boucle de
 * decision -- est a votre charge. Ne cherchez pas ces fonctions ici : elles
 * n'y sont pas, et c'est volontaire.
 *
 * Les verifications faites ici sont minimales (voir reconfort_io.cpp) ; les
 * validations manquantes font partie du travail demande.
 */
#ifndef RECONFORT_IO_HPP
#define RECONFORT_IO_HPP

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "json.hpp"

constexpr int RECONFORT_VERSION_ATTENDUE = 1;

/* Fichier d'entree absent, illisible, ou d'un type inattendu (equivalent
 * de l'exception ErreurFichier de la version Python). */
class ErreurFichier : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/* Chargent un fichier JSON et verifient son en-tete ("format"/"version").
 * Levent ErreurFichier en cas d'erreur. */
JsonValue charger_carte(const std::string &chemin);
JsonValue charger_dictionnaire(const std::string &chemin);
JsonValue charger_armoire(const std::string &chemin);
JsonValue charger_scenario(const std::string &chemin);

/* Decoupe un message en mots comparables au dictionnaire : minuscules,
 * accents retires, decoupage sur tout ce qui n'est pas une lettre.
 *
 *     normaliser("Je suis TERRIFIEE, vraiment !")
 *         -> {"je", "suis", "terrifiee", "vraiment"}
 *
 * Couvre les lettres accentuees courantes du francais (Latin-1) ;
 * contrairement a la version Python (module unicodedata), elle ne couvre
 * pas l'ensemble Unicode. */
std::vector<std::string> normaliser(const std::string &texte);

/* ---------------------------------------------------------------------
 * Trace
 * ------------------------------------------------------------------- */

extern const std::vector<std::string> RECONFORT_ACTIONS;
extern const std::vector<std::string> RECONFORT_DIRECTIONS;
extern const std::vector<std::string> RECONFORT_REPLIS;

/* Ce que le robot voit depuis sa position, vers "mur", "libre", "armoire",
 * "dictionnaire" ou "resident". std::nullopt signifie "non observe". */
struct Perception {
    std::optional<std::string> n, s, e, o;
};

class Trace {
public:
    Trace(const std::string &nom_carte, const std::string &nom_scenario,
          const std::vector<std::string> &equipe);

    /* Enregistre un pas de simulation.
     *
     * `demande`            : nullopt si le pas n'est rattache a aucune demande.
     * `position_*`         : position (ligne, colonne) du robot AVANT l'action.
     * `casier_*`           : position (ligne, colonne) du selecteur dans
     *                         l'armoire, AVANT l'action.
     * `perception`         : nullopt si non renseignee.
     * `contenu_casier`     : objet du casier courant si le robot est devant
     *                         l'armoire, nullopt sinon.
     *
     * Leve std::invalid_argument si l'action ou l'argument est invalide. */
    void ajouter_pas(std::optional<int> demande,
                     int position_ligne, int position_colonne,
                     int casier_ligne, int casier_colonne,
                     const std::string &action,
                     const std::optional<std::string> &argument,
                     const std::optional<Perception> &perception,
                     const std::optional<std::string> &contenu_casier,
                     const std::optional<std::string> &commentaire);

    /* Enregistre l'issue d'une demande, reussie ou non.
     *
     * En cas d'echec, `succes` vaut false et `motif_echec` doit expliquer
     * pourquoi en une chaine courte (par exemple "resident inaccessible").
     *
     * `casier_choisi_ligne`/`casier_choisi_colonne` : nullopt si aucun
     * casier n'a ete choisi.
     *
     * Leve std::invalid_argument si repli invalide ou echec sans motif. */
    void ajouter_livraison(int demande, const std::string &resident,
                           const std::optional<std::string> &emotion,
                           const std::optional<std::string> &intensite,
                           std::optional<int> casier_choisi_ligne,
                           std::optional<int> casier_choisi_colonne,
                           const std::string &repli,
                           const std::optional<std::string> &objet,
                           bool succes,
                           const std::optional<std::string> &motif_echec);

    /* Ecrit la trace en JSON UTF-8 indente (2 espaces). Cree le dossier au
     * besoin. Leve ErreurFichier si le fichier n'a pas pu etre ecrit. */
    void ecrire(const std::string &chemin) const;

private:
    std::string nom_carte_;
    std::string nom_scenario_;
    JsonValue equipe_;      /* tableau de chaines */
    JsonValue pas_;         /* tableau d'objets */
    JsonValue livraisons_;  /* tableau d'objets */
};

#endif /* RECONFORT_IO_HPP */

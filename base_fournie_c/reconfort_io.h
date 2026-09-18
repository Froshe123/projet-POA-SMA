/*
 * Base de code fournie -- projet "Robot de reconfort" (traduction en C).
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
 * Les verifications faites ici sont minimales (voir reconfort_io.c) ; les
 * validations manquantes font partie du travail demande.
 */
#ifndef RECONFORT_IO_H
#define RECONFORT_IO_H

#include <stdbool.h>
#include <stddef.h>

#include "json.h"

#define RECONFORT_VERSION_ATTENDUE 1
#define RECONFORT_ERREUR_TAILLE 512

/* Fichier d'entree absent, illisible, ou d'un type inattendu (equivalent
 * de l'exception ErreurFichier de la version Python). */
typedef struct {
    char message[RECONFORT_ERREUR_TAILLE];
} ErreurFichier;

/* Chargent un fichier JSON et verifient son en-tete ("format"/"version").
 * Retournent NULL en cas d'erreur et remplissent *erreur ; sinon
 * retournent une valeur JSON dont l'appelant est proprietaire
 * (a liberer avec json_free). */
JsonValue *charger_carte(const char *chemin, ErreurFichier *erreur);
JsonValue *charger_dictionnaire(const char *chemin, ErreurFichier *erreur);
JsonValue *charger_armoire(const char *chemin, ErreurFichier *erreur);
JsonValue *charger_scenario(const char *chemin, ErreurFichier *erreur);

/* Decoupe un message en mots comparables au dictionnaire : minuscules,
 * accents retires, decoupage sur tout ce qui n'est pas une lettre.
 *
 *     normaliser("Je suis TERRIFIEE, vraiment !")
 *         -> {"je", "suis", "terrifiee", "vraiment"}, *nb_mots = 4
 *
 * Retourne un tableau de chaines allouees (malloc), a liberer avec
 * normaliser_liberer. Couvre les lettres accentuees courantes du francais
 * (Latin-1) ; contrairement a la version Python (module unicodedata), elle
 * ne couvre pas l'ensemble Unicode. */
char **normaliser(const char *texte, int *nb_mots);
void normaliser_liberer(char **mots, int nb_mots);

/* ---------------------------------------------------------------------
 * Trace
 * ------------------------------------------------------------------- */

extern const char *const RECONFORT_ACTIONS[];
extern const int RECONFORT_NB_ACTIONS;
extern const char *const RECONFORT_DIRECTIONS[];
extern const int RECONFORT_NB_DIRECTIONS;
extern const char *const RECONFORT_REPLIS[];
extern const int RECONFORT_NB_REPLIS;

typedef struct Trace Trace;

/* Ce que le robot voit depuis sa position, vers "mur", "libre", "armoire",
 * "dictionnaire" ou "resident". Un pointeur NULL signifie "non observe". */
typedef struct {
    const char *n, *s, *e, *o;
} Perception;

Trace *trace_creer(const char *nom_carte, const char *nom_scenario,
                    const char *const *equipe, int nb_equipe);
void trace_detruire(Trace *trace);

/* Enregistre un pas de simulation.
 *
 * `demande`            : NULL si le pas n'est rattache a aucune demande.
 * `position_*`         : position (ligne, colonne) du robot AVANT l'action.
 * `casier_*`           : position (ligne, colonne) du selecteur dans
 *                         l'armoire, AVANT l'action.
 * `perception`         : NULL si non renseignee.
 * `contenu_casier`     : objet du casier courant si le robot est devant
 *                         l'armoire, NULL sinon.
 *
 * Retourne 0 en cas de succes, -1 si l'action ou l'argument est invalide
 * (message explicatif ecrit dans erreur). */
int trace_ajouter_pas(Trace *trace, const int *demande,
                       int position_ligne, int position_colonne,
                       int casier_ligne, int casier_colonne,
                       const char *action, const char *argument,
                       const Perception *perception,
                       const char *contenu_casier,
                       const char *commentaire,
                       char *erreur, size_t taille_erreur);

/* Enregistre l'issue d'une demande, reussie ou non.
 *
 * En cas d'echec, `succes` vaut false et `motif_echec` doit expliquer
 * pourquoi en une chaine courte (par exemple "resident inaccessible").
 *
 * `casier_choisi_ligne`/`casier_choisi_colonne` : NULL si aucun casier
 * n'a ete choisi.
 *
 * Retourne 0 en cas de succes, -1 si repli invalide ou echec sans motif. */
int trace_ajouter_livraison(Trace *trace, int demande, const char *resident,
                             const char *emotion, const char *intensite,
                             const int *casier_choisi_ligne,
                             const int *casier_choisi_colonne,
                             const char *repli, const char *objet,
                             bool succes, const char *motif_echec,
                             char *erreur, size_t taille_erreur);

/* Ecrit la trace en JSON UTF-8 indente (2 espaces). Cree le dossier au
 * besoin. Retourne true en cas de succes, false si le fichier n'a pas pu
 * etre ecrit (message dans *erreur). */
bool trace_ecrire(Trace *trace, const char *chemin, ErreurFichier *erreur);

#endif /* RECONFORT_IO_H */

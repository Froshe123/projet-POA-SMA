#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdbool.h>
#include "types.h"
#include "json.h"

/* Ce que le robot perçoit au début de chaque tour (énoncé 3.2). */
typedef struct {
    const char *voisins[4];       /* dans l'ordre N, S, E, O : "mur", "libre", "armoire",
                                     "dictionnaire" ou "resident" */
    bool devant_armoire;          /* le robot touche-t-il l'armoire ? */
    const char *contenu_casier;   /* objet du casier courant ; NULL s'il est vide
                                     ou si le robot n'est pas devant l'armoire */
} PerceptionRobot;

/* Les actions du robot */
typedef enum { AVANCER, CONSULTER, CHERCHER, PRENDRE, DONNER, ATTENDRE, FIN } TypeAction;

typedef struct {
    TypeAction type;
    char direction;   /* 'N', 'S', 'E' ou 'O' pour AVANCER et CHERCHER */
    char objet[64];   /* nom de l'objet pour PRENDRE et DONNER */
} Action;

/* ===================== Ce que le robot sait au départ ===================== */

#define MAX_RESIDENTS 32

/* Un résident tel que le robot le connaît : son identifiant et sa position. */
typedef struct {
    char id[16];   /* par exemple "R1" */
    Pos pos;
} ResidentConnu;

/* Tout ce que le robot sait AU DÉPART (énoncé 3.2). Ni la grille, ni les objets. */
typedef struct {
    int hauteur, largeur;
    Pos depart, armoire, dictionnaire;
    int nb_residents;
    ResidentConnu residents[MAX_RESIDENTS];
    Pos casier_depart;   /* position du sélecteur de l'armoire au départ */
} Connaissances;

/* Le bilan d'une demande, écrit dans la trace (énoncé 5.5). Une chaîne vide "" = null. */
typedef struct {
    char emotion[32];
    char intensite[32];
    bool a_un_casier;      /* false : "casier_choisi" sera écrit null */
    Pos casier_choisi;
    char repli[16];        /* "aucun", "intensite", "voisine_1" ou "voisine_2" */
    char objet[64];
    bool succes;
    char motif_echec[64];  /* "resident inaccessible", "armoire vide", "emotion indeterminee" */
} Bilan;

/* ===================== Le robot (Axel, agent.c) ===================== */

typedef struct Agent Agent;   /* le contenu de Agent est écrit dans agent.c */

Agent *agent_creer(const Connaissances *k, const JsonValue *dictionnaire);
void agent_nouvelle_demande(Agent *a, int numero, const char *resident, const char *message);
void agent_percevoir(Agent *a, const PerceptionRobot *p);   /* PERCEPTION -> MÉMOIRE */
Action agent_decider(Agent *a);                             /* DÉCISION (FIN = demande finie) */
void agent_appliquer(Agent *a, Action action);              /* mise à jour après l'action */
Bilan agent_bilan(const Agent *a);
void agent_detruire(Agent *a);

/* ===================== Le monde (Eliott, environnement.c) ===================== */

typedef struct Environnement Environnement;   /* le contenu est écrit dans environnement.c */

Environnement *env_creer(const JsonValue *carte, const JsonValue *armoire);
Connaissances env_connaissances(const Environnement *e);   /* ce que le robot a le droit de savoir */
void env_viser_resident(Environnement *e, Pos resident);   /* résident de la demande en cours */
PerceptionRobot env_percevoir(const Environnement *e);
bool env_executer(Environnement *e, Action action);        /* false si l'action est illégale */
Pos env_robot(const Environnement *e);
Pos env_selecteur(const Environnement *e);
void env_detruire(Environnement *e);

/* ===================== La roue (Eliott, roue.c ; utilisée par les deux) ===================== */

int roue_avancer(int i, int pas, int n);   /* avance de 'pas' crans sur une roue de n cases */
int roue_distance(int a, int b, int n);    /* nombre de crans par le côté le plus court */

#endif
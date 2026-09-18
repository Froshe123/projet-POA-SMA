/*
 * Demonstration de la base fournie -- projet "Robot de reconfort" (C).
 *
 * Ce programme ne resout rien. Il montre seulement :
 *   - le contrat d'appel attendu (quatre arguments) ;
 *   - comment charger les quatre fichiers d'entree ;
 *   - comment gerer proprement un fichier absent ou mal forme ;
 *   - comment produire un fichier de trace conforme.
 *
 * Le robot de cette demo n'avance pas, ne consulte rien, et echoue sur
 * toutes les demandes. C'est a vous d'ecrire celui qui reussit.
 *
 *     ./demo cartes/appartement_01.json cartes/scenario_01.json \
 *         donnees sorties/trace_01.json
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "reconfort_io.h"

static const char *AIDE =
    "Demonstration de la base fournie -- projet \"Robot de reconfort\".\n"
    "\n"
    "Ce programme ne resout rien. Il montre seulement :\n"
    "  - le contrat d'appel attendu (quatre arguments) ;\n"
    "  - comment charger les quatre fichiers d'entree ;\n"
    "  - comment gerer proprement un fichier absent ou mal forme ;\n"
    "  - comment produire un fichier de trace conforme.\n"
    "\n"
    "Le robot de cette demo n'avance pas, ne consulte rien, et echoue sur\n"
    "toutes les demandes. C'est a vous d'ecrire celui qui reussit.\n"
    "\n"
    "    ./demo cartes/appartement_01.json cartes/scenario_01.json \\\n"
    "        donnees sorties/trace_01.json";

static void afficher_paire(const char *etiquette, const JsonValue *paire) {
    int a = 0, b = 0;
    json_paire_entiers(paire, &a, &b);
    printf("%-16s [%d, %d]\n", etiquette, a, b);
}

int main(int argc, char **argv) {
    if (argc != 5) {
        puts(AIDE);
        return 2;
    }

    const char *chemin_carte = argv[1];
    const char *chemin_scenario = argv[2];
    const char *dossier_donnees = argv[3];
    const char *chemin_sortie = argv[4];

    ErreurFichier err;

    JsonValue *carte = charger_carte(chemin_carte, &err);
    if (!carte) {
        fprintf(stderr, "erreur de chargement : %s\n", err.message);
        return 1;
    }

    JsonValue *scenario = charger_scenario(chemin_scenario, &err);
    if (!scenario) {
        fprintf(stderr, "erreur de chargement : %s\n", err.message);
        json_free(carte);
        return 1;
    }

    char chemin_dico[4096];
    snprintf(chemin_dico, sizeof(chemin_dico), "%s/dictionnaire.json", dossier_donnees);
    JsonValue *dictionnaire = charger_dictionnaire(chemin_dico, &err);
    if (!dictionnaire) {
        fprintf(stderr, "erreur de chargement : %s\n", err.message);
        json_free(carte);
        json_free(scenario);
        return 1;
    }

    const char *nom_armoire = json_get_chaine(scenario, "armoire", "");
    char chemin_armoire[4096];
    snprintf(chemin_armoire, sizeof(chemin_armoire), "%s/%s.json", dossier_donnees, nom_armoire);
    JsonValue *armoire = charger_armoire(chemin_armoire, &err);
    if (!armoire) {
        fprintf(stderr, "erreur de chargement : %s\n", err.message);
        json_free(carte);
        json_free(scenario);
        json_free(dictionnaire);
        return 1;
    }

    JsonValue *dimensions = json_get(carte, "dimensions");
    printf("carte        : %s %dx%d, %zu residents\n",
           json_get_chaine(carte, "nom", ""),
           (int)json_get_nombre(dimensions, "hauteur", 0),
           (int)json_get_nombre(dimensions, "largeur", 0),
           json_taille(json_get(carte, "residents")));
    printf("scenario     : %s, %zu demandes\n",
           json_get_chaine(scenario, "nom", ""),
           json_taille(json_get(scenario, "demandes")));
    printf("dictionnaire : %zu entrees\n",
           json_taille(json_get(dictionnaire, "entrees")));
    printf("armoire      : %s, %zu casiers\n",
           json_get_chaine(armoire, "nom", ""),
           json_taille(json_get(armoire, "casiers")));
    printf("\n");

    /* Ce que voit le robot au depart : sa position, celle de l'armoire,
     * celle du dictionnaire, et celles des residents. Les murs, eux, ne
     * sont PAS connus a priori, et le contenu des casiers non plus (enonce
     * 3.2 et 3.3). Ne lisez ni carte["grille"], ni le champ "objet" des
     * casiers : votre agent n'y a pas droit. */
    afficher_paire("depart robot :", json_get(carte, "depart_robot"));
    afficher_paire("armoire      :", json_get(json_get(carte, "armoire"), "position"));
    afficher_paire("dictionnaire :", json_get(json_get(carte, "dictionnaire"), "position"));
    afficher_paire("casier depart:", json_get(armoire, "casier_depart"));

    const char *equipe[] = {"A completer", "A completer"};
    Trace *trace = trace_creer(json_get_chaine(carte, "nom", ""),
                                json_get_chaine(scenario, "nom", ""),
                                equipe, 2);

    int depart_l = 0, depart_c = 0;
    json_paire_entiers(json_get(carte, "depart_robot"), &depart_l, &depart_c);
    int casier_depart_l = 0, casier_depart_c = 0;
    json_paire_entiers(json_get(armoire, "casier_depart"), &casier_depart_l, &casier_depart_c);

    JsonValue *demandes = json_get(scenario, "demandes");
    for (size_t i = 0; i < json_taille(demandes); i++) {
        JsonValue *demande = json_index(demandes, i);
        int numero = (int)json_get_nombre(demande, "numero", 0);
        const char *resident = json_get_chaine(demande, "resident", "");
        const char *message = json_get_chaine(demande, "message", "");

        printf("\ndemande %d (%s) : %s\n", numero, resident, message);

        int nb_mots;
        char **mots = normaliser(message, &nb_mots);
        printf("  mots normalises : [");
        for (int m = 0; m < nb_mots; m++) {
            printf("%s'%s'", m ? ", " : "", mots[m]);
        }
        printf("]\n");
        normaliser_liberer(mots, nb_mots);

        char message_erreur[RECONFORT_ERREUR_TAILLE];
        trace_ajouter_pas(trace, &numero, depart_l, depart_c,
                           casier_depart_l, casier_depart_c, "ATTENDRE", NULL,
                           NULL, NULL, "robot de demonstration : ne fait rien",
                           message_erreur, sizeof(message_erreur));
        trace_ajouter_livraison(trace, numero, resident, NULL, NULL, NULL, NULL,
                                 "aucun", NULL, false, "agent non implemente",
                                 message_erreur, sizeof(message_erreur));
    }

    ErreurFichier err_ecriture;
    if (!trace_ecrire(trace, chemin_sortie, &err_ecriture)) {
        fprintf(stderr, "erreur d'ecriture : %s\n", err_ecriture.message);
        trace_detruire(trace);
        json_free(carte);
        json_free(scenario);
        json_free(dictionnaire);
        json_free(armoire);
        return 1;
    }
    printf("\ntrace ecrite : %s\n", chemin_sortie);

    trace_detruire(trace);
    json_free(carte);
    json_free(scenario);
    json_free(dictionnaire);
    json_free(armoire);
    return 0;
}

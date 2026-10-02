/*
 * Demonstration de la base fournie -- projet "Robot de reconfort" (C++).
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
#include <cstdio>
#include <iostream>
#include <string>

#include "json.hpp"
#include "reconfort_io.hpp"

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
    if (paire) paire->paire_entiers(a, b);
    std::printf("%-16s [%d, %d]\n", etiquette, a, b);
}

/* Taille d'un champ tableau, 0 s'il est absent. */
static std::size_t taille_champ(const JsonValue &objet, const std::string &cle) {
    const JsonValue *v = objet.get(cle);
    return v ? v->taille() : 0;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        std::puts(AIDE);
        return 2;
    }

    const std::string chemin_carte = argv[1];
    const std::string chemin_scenario = argv[2];
    const std::string dossier_donnees = argv[3];
    const std::string chemin_sortie = argv[4];

    JsonValue carte, scenario, dictionnaire, armoire;
    try {
        carte = charger_carte(chemin_carte);
        scenario = charger_scenario(chemin_scenario);
        dictionnaire = charger_dictionnaire(dossier_donnees + "/dictionnaire.json");
        std::string nom_armoire = scenario.get_chaine("armoire", "");
        armoire = charger_armoire(dossier_donnees + "/" + nom_armoire + ".json");
    } catch (const ErreurFichier &e) {
        std::cerr << "erreur de chargement : " << e.what() << "\n";
        return 1;
    }

    const JsonValue *dimensions = carte.get("dimensions");
    std::printf("carte        : %s %dx%d, %zu residents\n",
                carte.get_chaine("nom", "").c_str(),
                dimensions ? static_cast<int>(dimensions->get_nombre("hauteur", 0)) : 0,
                dimensions ? static_cast<int>(dimensions->get_nombre("largeur", 0)) : 0,
                taille_champ(carte, "residents"));
    std::printf("scenario     : %s, %zu demandes\n",
                scenario.get_chaine("nom", "").c_str(),
                taille_champ(scenario, "demandes"));
    std::printf("dictionnaire : %zu entrees\n",
                taille_champ(dictionnaire, "entrees"));
    std::printf("armoire      : %s, %zu casiers\n",
                armoire.get_chaine("nom", "").c_str(),
                taille_champ(armoire, "casiers"));
    std::printf("\n");

    /* Ce que voit le robot au depart : sa position, celle de l'armoire,
     * celle du dictionnaire, et celles des residents. Les murs, eux, ne
     * sont PAS connus a priori, et le contenu des casiers non plus (enonce
     * 3.2 et 3.3). Ne lisez ni carte["grille"], ni le champ "objet" des
     * casiers : votre agent n'y a pas droit. */
    const JsonValue *pos_armoire = carte.get("armoire");
    const JsonValue *pos_dictionnaire = carte.get("dictionnaire");
    afficher_paire("depart robot :", carte.get("depart_robot"));
    afficher_paire("armoire      :", pos_armoire ? pos_armoire->get("position") : nullptr);
    afficher_paire("dictionnaire :", pos_dictionnaire ? pos_dictionnaire->get("position") : nullptr);
    afficher_paire("casier depart:", armoire.get("casier_depart"));

    Trace trace(carte.get_chaine("nom", ""), scenario.get_chaine("nom", ""),
                {"A completer", "A completer"});

    int depart_l = 0, depart_c = 0;
    if (const JsonValue *d = carte.get("depart_robot")) d->paire_entiers(depart_l, depart_c);
    int casier_depart_l = 0, casier_depart_c = 0;
    if (const JsonValue *c = armoire.get("casier_depart")) c->paire_entiers(casier_depart_l, casier_depart_c);

    const JsonValue *demandes = scenario.get("demandes");
    for (std::size_t i = 0; demandes && i < demandes->taille(); i++) {
        const JsonValue *demande = demandes->index(i);
        int numero = static_cast<int>(demande->get_nombre("numero", 0));
        std::string resident = demande->get_chaine("resident", "");
        std::string message = demande->get_chaine("message", "");

        std::printf("\ndemande %d (%s) : %s\n", numero, resident.c_str(), message.c_str());

        std::vector<std::string> mots = normaliser(message);
        std::printf("  mots normalises : [");
        for (std::size_t m = 0; m < mots.size(); m++) {
            std::printf("%s'%s'", m ? ", " : "", mots[m].c_str());
        }
        std::printf("]\n");

        trace.ajouter_pas(numero, depart_l, depart_c,
                          casier_depart_l, casier_depart_c, "ATTENDRE", std::nullopt,
                          std::nullopt, std::nullopt, "robot de demonstration : ne fait rien");
        trace.ajouter_livraison(numero, resident, std::nullopt, std::nullopt,
                                std::nullopt, std::nullopt,
                                "aucun", std::nullopt, false, "agent non implemente");
    }

    try {
        trace.ecrire(chemin_sortie);
    } catch (const ErreurFichier &e) {
        std::cerr << "erreur d'ecriture : " << e.what() << "\n";
        return 1;
    }
    std::printf("\ntrace ecrite : %s\n", chemin_sortie.c_str());
    return 0;
}

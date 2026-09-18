#define _POSIX_C_SOURCE 200809L

#include "reconfort_io.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

/* ---------------------------------------------------------------------
 * Lecture
 * ------------------------------------------------------------------- */

/* Verifie que `donnees` (de `taille` octets) est de l'UTF-8 valide.
 * Retourne false et place dans *position_invalide l'octet fautif sinon.
 * Validation simplifiee (ne rejette pas les encodages surlongs), mais
 * suffisante pour detecter un fichier mal encode. */
static bool utf8_valide(const char *donnees, size_t taille, size_t *position_invalide) {
    size_t i = 0;
    while (i < taille) {
        unsigned char c = (unsigned char)donnees[i];
        int suite;
        if (c < 0x80) suite = 0;
        else if ((c & 0xE0) == 0xC0) suite = 1;
        else if ((c & 0xF0) == 0xE0) suite = 2;
        else if ((c & 0xF8) == 0xF0) suite = 3;
        else {
            *position_invalide = i;
            return false;
        }
        for (int k = 1; k <= suite; k++) {
            if (i + (size_t)k >= taille ||
                ((unsigned char)donnees[i + (size_t)k] & 0xC0) != 0x80) {
                *position_invalide = i;
                return false;
            }
        }
        i += (size_t)suite + 1;
    }
    return true;
}

static char *lire_fichier_texte(const char *chemin, ErreurFichier *erreur) {
    FILE *f = fopen(chemin, "rb");
    if (!f) {
        snprintf(erreur->message, sizeof(erreur->message),
                 "fichier introuvable : %s", chemin);
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        snprintf(erreur->message, sizeof(erreur->message),
                 "%s illisible", chemin);
        return NULL;
    }
    long taille = ftell(f);
    if (taille < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        snprintf(erreur->message, sizeof(erreur->message),
                 "%s illisible", chemin);
        return NULL;
    }

    char *texte = malloc((size_t)taille + 1);
    size_t lu = fread(texte, 1, (size_t)taille, f);
    bool erreur_lecture = ferror(f) != 0;
    fclose(f);
    if (erreur_lecture) {
        free(texte);
        snprintf(erreur->message, sizeof(erreur->message),
                 "%s illisible", chemin);
        return NULL;
    }
    texte[lu] = '\0';

    size_t position_invalide;
    if (!utf8_valide(texte, lu, &position_invalide)) {
        snprintf(erreur->message, sizeof(erreur->message),
                 "%s n'est pas encode en UTF-8 (octet %zu)", chemin,
                 position_invalide);
        free(texte);
        return NULL;
    }
    return texte;
}

/* Lit un fichier JSON UTF-8 et verifie son en-tete.
 *
 * Ce qui EST verifie ici :
 *   - le fichier existe et se lit en UTF-8 ;
 *   - son contenu est du JSON valide ;
 *   - la racine est un objet ;
 *   - les champs "format" et "version" sont presents et corrects.
 *
 * Ce qui n'est PAS verifie (a vous de le faire) :
 *   - la presence et le type de chacun des autres champs ;
 *   - la coherence des donnees (grille rectangulaire, positions dans les
 *     bornes, resident pose sur un mur, casier hors de l'armoire, emotion
 *     inconnue, resident cite par un scenario mais absent de la carte...).
 */
static JsonValue *_lire_json(const char *chemin, const char *format_attendu,
                              ErreurFichier *erreur) {
    char *texte = lire_fichier_texte(chemin, erreur);
    if (!texte) return NULL;

    char message_json[300];
    JsonValue *donnees = json_parse(texte, message_json, sizeof(message_json));
    free(texte);
    if (!donnees) {
        snprintf(erreur->message, sizeof(erreur->message),
                 "%s n'est pas un JSON valide : %s", chemin, message_json);
        return NULL;
    }

    if (!json_est_objet(donnees)) {
        snprintf(erreur->message, sizeof(erreur->message),
                 "%s : la racine doit etre un objet JSON", chemin);
        json_free(donnees);
        return NULL;
    }

    const char *format_trouve = json_get_chaine(donnees, "format", NULL);
    if (!format_trouve || strcmp(format_trouve, format_attendu) != 0) {
        if (format_trouve) {
            snprintf(erreur->message, sizeof(erreur->message),
                     "%s : format attendu '%s', trouve '%s'", chemin,
                     format_attendu, format_trouve);
        } else {
            snprintf(erreur->message, sizeof(erreur->message),
                     "%s : format attendu '%s', trouve None", chemin,
                     format_attendu);
        }
        json_free(donnees);
        return NULL;
    }

    JsonValue *version = json_get(donnees, "version");
    if (!version || version->type != JSON_NUMBER ||
        (int)version->donnee.number != RECONFORT_VERSION_ATTENDUE) {
        if (version && version->type == JSON_NUMBER) {
            snprintf(erreur->message, sizeof(erreur->message),
                     "%s : version %d attendue, trouve %g", chemin,
                     RECONFORT_VERSION_ATTENDUE, version->donnee.number);
        } else {
            snprintf(erreur->message, sizeof(erreur->message),
                     "%s : version %d attendue, trouve None", chemin,
                     RECONFORT_VERSION_ATTENDUE);
        }
        json_free(donnees);
        return NULL;
    }

    return donnees;
}

JsonValue *charger_carte(const char *chemin, ErreurFichier *erreur) {
    return _lire_json(chemin, "robot-reconfort/carte", erreur);
}

JsonValue *charger_dictionnaire(const char *chemin, ErreurFichier *erreur) {
    return _lire_json(chemin, "robot-reconfort/dictionnaire", erreur);
}

JsonValue *charger_armoire(const char *chemin, ErreurFichier *erreur) {
    return _lire_json(chemin, "robot-reconfort/armoire", erreur);
}

JsonValue *charger_scenario(const char *chemin, ErreurFichier *erreur) {
    return _lire_json(chemin, "robot-reconfort/scenario", erreur);
}

/* ---------------------------------------------------------------------
 * Normalisation des messages
 * ------------------------------------------------------------------- */

static int decoder_utf8(const char *s, unsigned int *codepoint) {
    unsigned char c0 = (unsigned char)s[0];
    if (c0 < 0x80) {
        *codepoint = c0;
        return 1;
    }
    if ((c0 & 0xE0) == 0xC0 && ((unsigned char)s[1] & 0xC0) == 0x80) {
        *codepoint = ((unsigned int)(c0 & 0x1F) << 6) |
                     ((unsigned int)s[1] & 0x3F);
        return 2;
    }
    if ((c0 & 0xF0) == 0xE0 && ((unsigned char)s[1] & 0xC0) == 0x80 &&
        ((unsigned char)s[2] & 0xC0) == 0x80) {
        *codepoint = ((unsigned int)(c0 & 0x0F) << 12) |
                     (((unsigned int)s[1] & 0x3F) << 6) |
                     ((unsigned int)s[2] & 0x3F);
        return 3;
    }
    if ((c0 & 0xF8) == 0xF0 && ((unsigned char)s[1] & 0xC0) == 0x80 &&
        ((unsigned char)s[2] & 0xC0) == 0x80 &&
        ((unsigned char)s[3] & 0xC0) == 0x80) {
        *codepoint = ((unsigned int)(c0 & 0x07) << 18) |
                     (((unsigned int)s[1] & 0x3F) << 12) |
                     (((unsigned int)s[2] & 0x3F) << 6) |
                     ((unsigned int)s[3] & 0x3F);
        return 4;
    }
    /* octet invalide isole : on avance d'un octet pour ne pas boucler */
    *codepoint = c0;
    return 1;
}

/* Marques diacritiques combinantes (accents deja decomposes) : ignorees,
 * comme le category(c) != "Mn" de la version Python. */
static bool est_combinant(unsigned int cp) {
    return cp >= 0x0300 && cp <= 0x036F;
}

/* Lettres latines courantes du francais avec leur equivalent ASCII. Couvre
 * Latin-1 Supplement ; contrairement a unicodedata, ne couvre pas tout
 * Unicode (limite documentee dans reconfort_io.h). */
static bool lettre_de_base(unsigned int cp, char *base) {
    if (cp < 128) {
        if (isalpha((int)cp)) {
            *base = (char)tolower((int)cp);
            return true;
        }
        return false;
    }
    switch (cp) {
        case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5:
        case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5:
            *base = 'a';
            return true;
        case 0xC7: case 0xE7:
            *base = 'c';
            return true;
        case 0xC8: case 0xC9: case 0xCA: case 0xCB:
        case 0xE8: case 0xE9: case 0xEA: case 0xEB:
            *base = 'e';
            return true;
        case 0xCC: case 0xCD: case 0xCE: case 0xCF:
        case 0xEC: case 0xED: case 0xEE: case 0xEF:
            *base = 'i';
            return true;
        case 0xD1: case 0xF1:
            *base = 'n';
            return true;
        case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6:
        case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6:
            *base = 'o';
            return true;
        case 0xD9: case 0xDA: case 0xDB: case 0xDC:
        case 0xF9: case 0xFA: case 0xFB: case 0xFC:
            *base = 'u';
            return true;
        case 0xDD: case 0xFD: case 0xFF:
            *base = 'y';
            return true;
        default:
            return false;
    }
}

char **normaliser(const char *texte, int *nb_mots) {
    size_t capacite_mots = 8;
    char **mots = malloc(capacite_mots * sizeof(char *));
    int n = 0;

    size_t capacite_mot = 32;
    char *mot = malloc(capacite_mot);
    size_t taille_mot = 0;

    const char *p = texte;
    while (*p) {
        unsigned int cp;
        p += decoder_utf8(p, &cp);

        if (est_combinant(cp)) continue;

        char base;
        if (lettre_de_base(cp, &base)) {
            if (taille_mot + 1 >= capacite_mot) {
                capacite_mot *= 2;
                mot = realloc(mot, capacite_mot);
            }
            mot[taille_mot++] = base;
        } else if (taille_mot > 0) {
            mot[taille_mot] = '\0';
            if ((size_t)n == capacite_mots) {
                capacite_mots *= 2;
                mots = realloc(mots, capacite_mots * sizeof(char *));
            }
            mots[n++] = strdup(mot);
            taille_mot = 0;
        }
    }
    if (taille_mot > 0) {
        mot[taille_mot] = '\0';
        if ((size_t)n == capacite_mots) {
            capacite_mots *= 2;
            mots = realloc(mots, capacite_mots * sizeof(char *));
        }
        mots[n++] = strdup(mot);
    }
    free(mot);

    *nb_mots = n;
    return mots;
}

void normaliser_liberer(char **mots, int nb_mots) {
    for (int i = 0; i < nb_mots; i++) free(mots[i]);
    free(mots);
}

/* ---------------------------------------------------------------------
 * Ecriture de la trace
 * ------------------------------------------------------------------- */

const char *const RECONFORT_ACTIONS[] = {
    "AVANCER", "CONSULTER", "CHERCHER", "PRENDRE", "DONNER", "ATTENDRE"};
const int RECONFORT_NB_ACTIONS = 6;

const char *const RECONFORT_DIRECTIONS[] = {"N", "S", "E", "O"};
const int RECONFORT_NB_DIRECTIONS = 4;

const char *const RECONFORT_REPLIS[] = {"aucun", "intensite", "voisine_1",
                                         "voisine_2"};
const int RECONFORT_NB_REPLIS = 4;

struct Trace {
    char *nom_carte;
    char *nom_scenario;
    JsonValue *equipe;      /* JSON_ARRAY de chaines */
    JsonValue *pas;         /* JSON_ARRAY d'objets */
    JsonValue *livraisons;  /* JSON_ARRAY d'objets */
};

static bool dans_liste(const char *valeur, const char *const *liste, int n) {
    for (int i = 0; i < n; i++) {
        if (strcmp(valeur, liste[i]) == 0) return true;
    }
    return false;
}

static void joindre_liste(char *buf, size_t taille, const char *const *liste, int n) {
    buf[0] = '\0';
    for (int i = 0; i < n; i++) {
        strncat(buf, liste[i], taille - strlen(buf) - 1);
        if (i + 1 < n) strncat(buf, ", ", taille - strlen(buf) - 1);
    }
}

Trace *trace_creer(const char *nom_carte, const char *nom_scenario,
                    const char *const *equipe, int nb_equipe) {
    Trace *trace = malloc(sizeof(Trace));
    trace->nom_carte = strdup(nom_carte);
    trace->nom_scenario = strdup(nom_scenario);

    trace->equipe = json_nouveau_tableau();
    for (int i = 0; i < nb_equipe; i++) {
        json_tableau_ajoute(trace->equipe, json_nouvelle_chaine(equipe[i]));
    }

    trace->pas = json_nouveau_tableau();
    trace->livraisons = json_nouveau_tableau();
    return trace;
}

void trace_detruire(Trace *trace) {
    if (!trace) return;
    free(trace->nom_carte);
    free(trace->nom_scenario);
    json_free(trace->equipe);
    json_free(trace->pas);
    json_free(trace->livraisons);
    free(trace);
}

int trace_ajouter_pas(Trace *trace, const int *demande,
                       int position_ligne, int position_colonne,
                       int casier_ligne, int casier_colonne,
                       const char *action, const char *argument,
                       const Perception *perception,
                       const char *contenu_casier,
                       const char *commentaire,
                       char *erreur, size_t taille_erreur) {
    if (!dans_liste(action, RECONFORT_ACTIONS, RECONFORT_NB_ACTIONS)) {
        char liste[128];
        joindre_liste(liste, sizeof(liste), RECONFORT_ACTIONS, RECONFORT_NB_ACTIONS);
        snprintf(erreur, taille_erreur,
                 "action inconnue '%s' (attendu : %s)", action, liste);
        return -1;
    }
    bool avancer_ou_chercher = strcmp(action, "AVANCER") == 0 ||
                                strcmp(action, "CHERCHER") == 0;
    if (avancer_ou_chercher &&
        (!argument || !dans_liste(argument, RECONFORT_DIRECTIONS, RECONFORT_NB_DIRECTIONS))) {
        char liste[32];
        joindre_liste(liste, sizeof(liste), RECONFORT_DIRECTIONS, RECONFORT_NB_DIRECTIONS);
        snprintf(erreur, taille_erreur,
                 "%s attend une direction parmi {%s}, pas '%s'", action, liste,
                 argument ? argument : "None");
        return -1;
    }

    JsonValue *entree = json_nouvel_objet();
    json_objet_set(entree, "t", json_nouveau_nombre((double)json_taille(trace->pas)));
    json_objet_set(entree, "demande",
                   demande ? json_nouveau_nombre(*demande) : json_nouveau_nul());

    JsonValue *pos = json_nouveau_tableau();
    json_tableau_ajoute(pos, json_nouveau_nombre(position_ligne));
    json_tableau_ajoute(pos, json_nouveau_nombre(position_colonne));
    json_objet_set(entree, "position", pos);

    JsonValue *cas = json_nouveau_tableau();
    json_tableau_ajoute(cas, json_nouveau_nombre(casier_ligne));
    json_tableau_ajoute(cas, json_nouveau_nombre(casier_colonne));
    json_objet_set(entree, "casier", cas);

    json_objet_set(entree, "action", json_nouvelle_chaine(action));
    json_objet_set(entree, "argument", json_nouvelle_chaine(argument));

    if (perception) {
        JsonValue *perc = json_nouvel_objet();
        json_objet_set(perc, "N", json_nouvelle_chaine(perception->n));
        json_objet_set(perc, "S", json_nouvelle_chaine(perception->s));
        json_objet_set(perc, "E", json_nouvelle_chaine(perception->e));
        json_objet_set(perc, "O", json_nouvelle_chaine(perception->o));
        json_objet_set(entree, "perception", perc);
    } else {
        json_objet_set(entree, "perception", json_nouveau_nul());
    }

    json_objet_set(entree, "contenu_casier", json_nouvelle_chaine(contenu_casier));
    json_objet_set(entree, "commentaire", json_nouvelle_chaine(commentaire));

    json_tableau_ajoute(trace->pas, entree);
    return 0;
}

int trace_ajouter_livraison(Trace *trace, int demande, const char *resident,
                             const char *emotion, const char *intensite,
                             const int *casier_choisi_ligne,
                             const int *casier_choisi_colonne,
                             const char *repli, const char *objet,
                             bool succes, const char *motif_echec,
                             char *erreur, size_t taille_erreur) {
    if (!dans_liste(repli, RECONFORT_REPLIS, RECONFORT_NB_REPLIS)) {
        char liste[64];
        joindre_liste(liste, sizeof(liste), RECONFORT_REPLIS, RECONFORT_NB_REPLIS);
        snprintf(erreur, taille_erreur,
                 "repli inconnu '%s' (attendu : %s)", repli, liste);
        return -1;
    }
    if (!succes && (!motif_echec || motif_echec[0] == '\0')) {
        snprintf(erreur, taille_erreur,
                 "un echec doit etre accompagne d'un motif_echec");
        return -1;
    }

    int pas_utilises = 0;
    for (size_t i = 0; i < json_taille(trace->pas); i++) {
        JsonValue *p = json_index(trace->pas, i);
        JsonValue *d = json_get(p, "demande");
        if (d && d->type == JSON_NUMBER && (int)d->donnee.number == demande) {
            pas_utilises++;
        }
    }

    JsonValue *entree = json_nouvel_objet();
    json_objet_set(entree, "demande", json_nouveau_nombre(demande));
    json_objet_set(entree, "resident", json_nouvelle_chaine(resident));
    json_objet_set(entree, "emotion", json_nouvelle_chaine(emotion));
    json_objet_set(entree, "intensite", json_nouvelle_chaine(intensite));

    if (casier_choisi_ligne && casier_choisi_colonne) {
        JsonValue *cas = json_nouveau_tableau();
        json_tableau_ajoute(cas, json_nouveau_nombre(*casier_choisi_ligne));
        json_tableau_ajoute(cas, json_nouveau_nombre(*casier_choisi_colonne));
        json_objet_set(entree, "casier_choisi", cas);
    } else {
        json_objet_set(entree, "casier_choisi", json_nouveau_nul());
    }

    json_objet_set(entree, "repli", json_nouvelle_chaine(repli));
    json_objet_set(entree, "objet", json_nouvelle_chaine(objet));
    json_objet_set(entree, "pas_utilises", json_nouveau_nombre(pas_utilises));
    json_objet_set(entree, "succes", json_nouveau_bool(succes));
    json_objet_set(entree, "motif_echec", json_nouvelle_chaine(motif_echec));

    json_tableau_ajoute(trace->livraisons, entree);
    return 0;
}

static void creer_dossiers_parent(const char *chemin) {
    char tampon[4096];
    snprintf(tampon, sizeof(tampon), "%s", chemin);

    for (char *p = tampon + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tampon, 0777);
            *p = '/';
        }
    }
}

bool trace_ecrire(Trace *trace, const char *chemin, ErreurFichier *erreur) {
    creer_dossiers_parent(chemin);

    int reussies = 0;
    for (size_t i = 0; i < json_taille(trace->livraisons); i++) {
        JsonValue *l = json_index(trace->livraisons, i);
        JsonValue *s = json_get(l, "succes");
        if (s && s->type == JSON_BOOL && s->donnee.boolean) reussies++;
    }
    int total_livraisons = (int)json_taille(trace->livraisons);

    JsonValue *racine = json_nouvel_objet();
    json_objet_set(racine, "format", json_nouvelle_chaine("robot-reconfort/trace"));
    json_objet_set(racine, "version", json_nouveau_nombre(RECONFORT_VERSION_ATTENDUE));
    json_objet_set(racine, "carte", json_nouvelle_chaine(trace->nom_carte));
    json_objet_set(racine, "scenario", json_nouvelle_chaine(trace->nom_scenario));
    json_objet_set(racine, "equipe", json_clone(trace->equipe));
    json_objet_set(racine, "pas", json_clone(trace->pas));
    json_objet_set(racine, "livraisons", json_clone(trace->livraisons));

    JsonValue *resume = json_nouvel_objet();
    json_objet_set(resume, "demandes", json_nouveau_nombre(total_livraisons));
    json_objet_set(resume, "reussies", json_nouveau_nombre(reussies));
    json_objet_set(resume, "echecs", json_nouveau_nombre(total_livraisons - reussies));
    json_objet_set(resume, "pas_total", json_nouveau_nombre((double)json_taille(trace->pas)));
    json_objet_set(racine, "resume", resume);

    char *texte = json_dump(racine);
    json_free(racine);

    FILE *f = fopen(chemin, "wb");
    if (!f) {
        snprintf(erreur->message, sizeof(erreur->message),
                 "impossible d'ecrire %s : %s", chemin, strerror(errno));
        free(texte);
        return false;
    }
    fwrite(texte, 1, strlen(texte), f);
    fputc('\n', f);
    fclose(f);
    free(texte);
    return true;
}

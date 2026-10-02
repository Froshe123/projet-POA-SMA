#include "reconfort_io.hpp"

#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

/* ---------------------------------------------------------------------
 * Lecture
 * ------------------------------------------------------------------- */

/* Verifie que `donnees` est de l'UTF-8 valide. Retourne false et place
 * dans position_invalide l'octet fautif sinon. Validation simplifiee (ne
 * rejette pas les encodages surlongs), mais suffisante pour detecter un
 * fichier mal encode. */
static bool utf8_valide(const std::string &donnees, std::size_t &position_invalide) {
    std::size_t taille = donnees.size();
    std::size_t i = 0;
    while (i < taille) {
        unsigned char c = static_cast<unsigned char>(donnees[i]);
        int suite;
        if (c < 0x80) suite = 0;
        else if ((c & 0xE0) == 0xC0) suite = 1;
        else if ((c & 0xF0) == 0xE0) suite = 2;
        else if ((c & 0xF8) == 0xF0) suite = 3;
        else {
            position_invalide = i;
            return false;
        }
        for (int k = 1; k <= suite; k++) {
            if (i + static_cast<std::size_t>(k) >= taille ||
                (static_cast<unsigned char>(donnees[i + static_cast<std::size_t>(k)]) & 0xC0) != 0x80) {
                position_invalide = i;
                return false;
            }
        }
        i += static_cast<std::size_t>(suite) + 1;
    }
    return true;
}

static std::string lire_fichier_texte(const std::string &chemin) {
    std::ifstream f(chemin, std::ios::binary);
    if (!f) {
        throw ErreurFichier("fichier introuvable : " + chemin);
    }

    std::string texte((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());
    if (f.bad()) {
        throw ErreurFichier(chemin + " illisible");
    }

    std::size_t position_invalide;
    if (!utf8_valide(texte, position_invalide)) {
        throw ErreurFichier(chemin + " n'est pas encode en UTF-8 (octet " +
                            std::to_string(position_invalide) + ")");
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
static JsonValue lire_json(const std::string &chemin, const std::string &format_attendu) {
    std::string texte = lire_fichier_texte(chemin);

    JsonValue donnees;
    try {
        donnees = JsonValue::parse(texte);
    } catch (const ErreurJson &e) {
        throw ErreurFichier(chemin + " n'est pas un JSON valide : " + e.what());
    }

    if (!donnees.est_objet()) {
        throw ErreurFichier(chemin + " : la racine doit etre un objet JSON");
    }

    const JsonValue *format = donnees.get("format");
    const std::string *format_trouve = format ? format->chaine() : nullptr;
    if (!format_trouve || *format_trouve != format_attendu) {
        throw ErreurFichier(chemin + " : format attendu '" + format_attendu +
                            "', trouve " +
                            (format_trouve ? "'" + *format_trouve + "'" : "None"));
    }

    const JsonValue *version = donnees.get("version");
    if (!version || !version->est_nombre() ||
        static_cast<int>(version->nombre()) != RECONFORT_VERSION_ATTENDUE) {
        std::ostringstream message;
        message << chemin << " : version " << RECONFORT_VERSION_ATTENDUE
                << " attendue, trouve ";
        if (version && version->est_nombre()) message << version->nombre();
        else message << "None";
        throw ErreurFichier(message.str());
    }

    return donnees;
}

JsonValue charger_carte(const std::string &chemin) {
    return lire_json(chemin, "robot-reconfort/carte");
}

JsonValue charger_dictionnaire(const std::string &chemin) {
    return lire_json(chemin, "robot-reconfort/dictionnaire");
}

JsonValue charger_armoire(const std::string &chemin) {
    return lire_json(chemin, "robot-reconfort/armoire");
}

JsonValue charger_scenario(const std::string &chemin) {
    return lire_json(chemin, "robot-reconfort/scenario");
}

/* ---------------------------------------------------------------------
 * Normalisation des messages
 * ------------------------------------------------------------------- */

static int decoder_utf8(const char *s, unsigned int &codepoint) {
    unsigned char c0 = static_cast<unsigned char>(s[0]);
    unsigned char c1 = c0 ? static_cast<unsigned char>(s[1]) : 0;
    unsigned char c2 = c1 ? static_cast<unsigned char>(s[2]) : 0;
    unsigned char c3 = c2 ? static_cast<unsigned char>(s[3]) : 0;
    if (c0 < 0x80) {
        codepoint = c0;
        return 1;
    }
    if ((c0 & 0xE0) == 0xC0 && (c1 & 0xC0) == 0x80) {
        codepoint = (static_cast<unsigned int>(c0 & 0x1F) << 6) | (c1 & 0x3Fu);
        return 2;
    }
    if ((c0 & 0xF0) == 0xE0 && (c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80) {
        codepoint = (static_cast<unsigned int>(c0 & 0x0F) << 12) |
                    ((c1 & 0x3Fu) << 6) | (c2 & 0x3Fu);
        return 3;
    }
    if ((c0 & 0xF8) == 0xF0 && (c1 & 0xC0) == 0x80 && (c2 & 0xC0) == 0x80 &&
        (c3 & 0xC0) == 0x80) {
        codepoint = (static_cast<unsigned int>(c0 & 0x07) << 18) |
                    ((c1 & 0x3Fu) << 12) | ((c2 & 0x3Fu) << 6) | (c3 & 0x3Fu);
        return 4;
    }
    /* octet invalide isole : on avance d'un octet pour ne pas boucler */
    codepoint = c0;
    return 1;
}

/* Marques diacritiques combinantes (accents deja decomposes) : ignorees,
 * comme le category(c) != "Mn" de la version Python. */
static bool est_combinant(unsigned int cp) {
    return cp >= 0x0300 && cp <= 0x036F;
}

/* Lettres latines courantes du francais avec leur equivalent ASCII. Couvre
 * Latin-1 Supplement ; contrairement a unicodedata, ne couvre pas tout
 * Unicode (limite documentee dans reconfort_io.hpp). */
static bool lettre_de_base(unsigned int cp, char &base) {
    if (cp < 128) {
        if (std::isalpha(static_cast<int>(cp))) {
            base = static_cast<char>(std::tolower(static_cast<int>(cp)));
            return true;
        }
        return false;
    }
    switch (cp) {
        case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5:
        case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5:
            base = 'a';
            return true;
        case 0xC7: case 0xE7:
            base = 'c';
            return true;
        case 0xC8: case 0xC9: case 0xCA: case 0xCB:
        case 0xE8: case 0xE9: case 0xEA: case 0xEB:
            base = 'e';
            return true;
        case 0xCC: case 0xCD: case 0xCE: case 0xCF:
        case 0xEC: case 0xED: case 0xEE: case 0xEF:
            base = 'i';
            return true;
        case 0xD1: case 0xF1:
            base = 'n';
            return true;
        case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6:
        case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6:
            base = 'o';
            return true;
        case 0xD9: case 0xDA: case 0xDB: case 0xDC:
        case 0xF9: case 0xFA: case 0xFB: case 0xFC:
            base = 'u';
            return true;
        case 0xDD: case 0xFD: case 0xFF:
            base = 'y';
            return true;
        default:
            return false;
    }
}

std::vector<std::string> normaliser(const std::string &texte) {
    std::vector<std::string> mots;
    std::string mot;

    const char *p = texte.c_str();
    while (*p) {
        unsigned int cp;
        p += decoder_utf8(p, cp);

        if (est_combinant(cp)) continue;

        char base;
        if (lettre_de_base(cp, base)) {
            mot += base;
        } else if (!mot.empty()) {
            mots.push_back(mot);
            mot.clear();
        }
    }
    if (!mot.empty()) mots.push_back(mot);
    return mots;
}

/* ---------------------------------------------------------------------
 * Ecriture de la trace
 * ------------------------------------------------------------------- */

const std::vector<std::string> RECONFORT_ACTIONS = {
    "AVANCER", "CONSULTER", "CHERCHER", "PRENDRE", "DONNER", "ATTENDRE"};

const std::vector<std::string> RECONFORT_DIRECTIONS = {"N", "S", "E", "O"};

const std::vector<std::string> RECONFORT_REPLIS = {"aucun", "intensite", "voisine_1",
                                                    "voisine_2"};

static bool dans_liste(const std::string &valeur, const std::vector<std::string> &liste) {
    for (const std::string &element : liste) {
        if (valeur == element) return true;
    }
    return false;
}

static std::string joindre_liste(const std::vector<std::string> &liste) {
    std::string resultat;
    for (std::size_t i = 0; i < liste.size(); i++) {
        resultat += liste[i];
        if (i + 1 < liste.size()) resultat += ", ";
    }
    return resultat;
}

static JsonValue paire(int a, int b) {
    JsonValue t = JsonValue::tableau();
    t.ajoute(a);
    t.ajoute(b);
    return t;
}

Trace::Trace(const std::string &nom_carte, const std::string &nom_scenario,
             const std::vector<std::string> &equipe)
    : nom_carte_(nom_carte), nom_scenario_(nom_scenario),
      equipe_(JsonValue::tableau()), pas_(JsonValue::tableau()),
      livraisons_(JsonValue::tableau()) {
    for (const std::string &membre : equipe) equipe_.ajoute(membre);
}

void Trace::ajouter_pas(std::optional<int> demande,
                        int position_ligne, int position_colonne,
                        int casier_ligne, int casier_colonne,
                        const std::string &action,
                        const std::optional<std::string> &argument,
                        const std::optional<Perception> &perception,
                        const std::optional<std::string> &contenu_casier,
                        const std::optional<std::string> &commentaire) {
    if (!dans_liste(action, RECONFORT_ACTIONS)) {
        throw std::invalid_argument("action inconnue '" + action + "' (attendu : " +
                                    joindre_liste(RECONFORT_ACTIONS) + ")");
    }
    bool avancer_ou_chercher = action == "AVANCER" || action == "CHERCHER";
    if (avancer_ou_chercher && (!argument || !dans_liste(*argument, RECONFORT_DIRECTIONS))) {
        throw std::invalid_argument(action + " attend une direction parmi {" +
                                    joindre_liste(RECONFORT_DIRECTIONS) + "}, pas '" +
                                    (argument ? *argument : "None") + "'");
    }

    JsonValue entree = JsonValue::objet();
    entree.set("t", static_cast<int>(pas_.taille()));
    entree.set("demande", demande ? JsonValue(*demande) : JsonValue());
    entree.set("position", paire(position_ligne, position_colonne));
    entree.set("casier", paire(casier_ligne, casier_colonne));
    entree.set("action", action);
    entree.set("argument", argument);

    if (perception) {
        JsonValue perc = JsonValue::objet();
        perc.set("N", perception->n);
        perc.set("S", perception->s);
        perc.set("E", perception->e);
        perc.set("O", perception->o);
        entree.set("perception", perc);
    } else {
        entree.set("perception", JsonValue());
    }

    entree.set("contenu_casier", contenu_casier);
    entree.set("commentaire", commentaire);

    pas_.ajoute(entree);
}

void Trace::ajouter_livraison(int demande, const std::string &resident,
                              const std::optional<std::string> &emotion,
                              const std::optional<std::string> &intensite,
                              std::optional<int> casier_choisi_ligne,
                              std::optional<int> casier_choisi_colonne,
                              const std::string &repli,
                              const std::optional<std::string> &objet,
                              bool succes,
                              const std::optional<std::string> &motif_echec) {
    if (!dans_liste(repli, RECONFORT_REPLIS)) {
        throw std::invalid_argument("repli inconnu '" + repli + "' (attendu : " +
                                    joindre_liste(RECONFORT_REPLIS) + ")");
    }
    if (!succes && (!motif_echec || motif_echec->empty())) {
        throw std::invalid_argument("un echec doit etre accompagne d'un motif_echec");
    }

    int pas_utilises = 0;
    for (std::size_t i = 0; i < pas_.taille(); i++) {
        const JsonValue *d = pas_.index(i)->get("demande");
        if (d && d->est_nombre() && static_cast<int>(d->nombre()) == demande) {
            pas_utilises++;
        }
    }

    JsonValue entree = JsonValue::objet();
    entree.set("demande", demande);
    entree.set("resident", resident);
    entree.set("emotion", emotion);
    entree.set("intensite", intensite);

    if (casier_choisi_ligne && casier_choisi_colonne) {
        entree.set("casier_choisi", paire(*casier_choisi_ligne, *casier_choisi_colonne));
    } else {
        entree.set("casier_choisi", JsonValue());
    }

    entree.set("repli", repli);
    entree.set("objet", objet);
    entree.set("pas_utilises", pas_utilises);
    entree.set("succes", succes);
    entree.set("motif_echec", motif_echec);

    livraisons_.ajoute(entree);
}

void Trace::ecrire(const std::string &chemin) const {
    std::filesystem::path parent = std::filesystem::path(chemin).parent_path();
    if (!parent.empty()) {
        std::error_code ignore;
        std::filesystem::create_directories(parent, ignore);
    }

    int reussies = 0;
    for (std::size_t i = 0; i < livraisons_.taille(); i++) {
        const JsonValue *s = livraisons_.index(i)->get("succes");
        if (s && s->booleen()) reussies++;
    }
    int total_livraisons = static_cast<int>(livraisons_.taille());

    JsonValue racine = JsonValue::objet();
    racine.set("format", "robot-reconfort/trace");
    racine.set("version", RECONFORT_VERSION_ATTENDUE);
    racine.set("carte", nom_carte_);
    racine.set("scenario", nom_scenario_);
    racine.set("equipe", equipe_);
    racine.set("pas", pas_);
    racine.set("livraisons", livraisons_);

    JsonValue resume = JsonValue::objet();
    resume.set("demandes", total_livraisons);
    resume.set("reussies", reussies);
    resume.set("echecs", total_livraisons - reussies);
    resume.set("pas_total", static_cast<int>(pas_.taille()));
    racine.set("resume", resume);

    std::string texte = racine.dump();

    std::ofstream f(chemin, std::ios::binary);
    if (!f) {
        throw ErreurFichier("impossible d'ecrire " + chemin + " : " + std::strerror(errno));
    }
    f << texte << '\n';
}

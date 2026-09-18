#define _POSIX_C_SOURCE 200809L

#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------- */
/* Construction / destruction                                          */
/* ------------------------------------------------------------------- */

static JsonValue *json_nouveau(JsonType type) {
    JsonValue *v = calloc(1, sizeof(JsonValue));
    v->type = type;
    return v;
}

JsonValue *json_nouvel_objet(void) { return json_nouveau(JSON_OBJECT); }
JsonValue *json_nouveau_tableau(void) { return json_nouveau(JSON_ARRAY); }

JsonValue *json_nouvelle_chaine(const char *s) {
    if (!s) return json_nouveau(JSON_NULL);
    JsonValue *v = json_nouveau(JSON_STRING);
    v->donnee.string = strdup(s);
    return v;
}

JsonValue *json_nouveau_nombre(double n) {
    JsonValue *v = json_nouveau(JSON_NUMBER);
    v->donnee.number = n;
    return v;
}

JsonValue *json_nouveau_bool(bool b) {
    JsonValue *v = json_nouveau(JSON_BOOL);
    v->donnee.boolean = b;
    return v;
}

JsonValue *json_nouveau_nul(void) { return json_nouveau(JSON_NULL); }

void json_free(JsonValue *v) {
    if (!v) return;
    switch (v->type) {
        case JSON_STRING:
            free(v->donnee.string);
            break;
        case JSON_ARRAY:
            for (size_t i = 0; i < v->donnee.array.count; i++) {
                json_free(v->donnee.array.items[i]);
            }
            free(v->donnee.array.items);
            break;
        case JSON_OBJECT:
            for (size_t i = 0; i < v->donnee.object.count; i++) {
                free(v->donnee.object.cles[i]);
                json_free(v->donnee.object.valeurs[i]);
            }
            free(v->donnee.object.cles);
            free(v->donnee.object.valeurs);
            break;
        default:
            break;
    }
    free(v);
}

void json_objet_set(JsonValue *objet, const char *cle, JsonValue *valeur) {
    if (!objet || objet->type != JSON_OBJECT) return;
    size_t n = objet->donnee.object.count;
    if (n == objet->donnee.object.capacite) {
        size_t nouvelle_capacite = n ? n * 2 : 8;
        objet->donnee.object.cles = realloc(
            objet->donnee.object.cles, nouvelle_capacite * sizeof(char *));
        objet->donnee.object.valeurs = realloc(
            objet->donnee.object.valeurs, nouvelle_capacite * sizeof(JsonValue *));
        objet->donnee.object.capacite = nouvelle_capacite;
    }
    objet->donnee.object.cles[n] = strdup(cle);
    objet->donnee.object.valeurs[n] = valeur;
    objet->donnee.object.count = n + 1;
}

void json_tableau_ajoute(JsonValue *tableau, JsonValue *valeur) {
    if (!tableau || tableau->type != JSON_ARRAY) return;
    size_t n = tableau->donnee.array.count;
    if (n == tableau->donnee.array.capacite) {
        size_t nouvelle_capacite = n ? n * 2 : 8;
        tableau->donnee.array.items = realloc(
            tableau->donnee.array.items, nouvelle_capacite * sizeof(JsonValue *));
        tableau->donnee.array.capacite = nouvelle_capacite;
    }
    tableau->donnee.array.items[n] = valeur;
    tableau->donnee.array.count = n + 1;
}

JsonValue *json_clone(const JsonValue *v) {
    if (!v) return NULL;
    switch (v->type) {
        case JSON_NULL:
            return json_nouveau_nul();
        case JSON_BOOL:
            return json_nouveau_bool(v->donnee.boolean);
        case JSON_NUMBER:
            return json_nouveau_nombre(v->donnee.number);
        case JSON_STRING:
            return json_nouvelle_chaine(v->donnee.string);
        case JSON_ARRAY: {
            JsonValue *copie = json_nouveau_tableau();
            for (size_t i = 0; i < v->donnee.array.count; i++) {
                json_tableau_ajoute(copie, json_clone(v->donnee.array.items[i]));
            }
            return copie;
        }
        case JSON_OBJECT: {
            JsonValue *copie = json_nouvel_objet();
            for (size_t i = 0; i < v->donnee.object.count; i++) {
                json_objet_set(copie, v->donnee.object.cles[i],
                                json_clone(v->donnee.object.valeurs[i]));
            }
            return copie;
        }
    }
    return json_nouveau_nul();
}

/* ------------------------------------------------------------------- */
/* Acces                                                                */
/* ------------------------------------------------------------------- */

bool json_est_objet(const JsonValue *v) { return v && v->type == JSON_OBJECT; }
bool json_est_tableau(const JsonValue *v) { return v && v->type == JSON_ARRAY; }
bool json_est_chaine(const JsonValue *v) { return v && v->type == JSON_STRING; }
bool json_est_nul(const JsonValue *v) { return !v || v->type == JSON_NULL; }

JsonValue *json_get(const JsonValue *objet, const char *cle) {
    if (!json_est_objet(objet)) return NULL;
    for (size_t i = 0; i < objet->donnee.object.count; i++) {
        if (strcmp(objet->donnee.object.cles[i], cle) == 0) {
            return objet->donnee.object.valeurs[i];
        }
    }
    return NULL;
}

const char *json_get_chaine(const JsonValue *objet, const char *cle, const char *defaut) {
    JsonValue *v = json_get(objet, cle);
    return json_est_chaine(v) ? v->donnee.string : defaut;
}

double json_get_nombre(const JsonValue *objet, const char *cle, double defaut) {
    JsonValue *v = json_get(objet, cle);
    return (v && v->type == JSON_NUMBER) ? v->donnee.number : defaut;
}

size_t json_taille(const JsonValue *v) {
    if (!v) return 0;
    if (v->type == JSON_ARRAY) return v->donnee.array.count;
    if (v->type == JSON_OBJECT) return v->donnee.object.count;
    return 0;
}

JsonValue *json_index(const JsonValue *tableau, size_t i) {
    if (!json_est_tableau(tableau) || i >= tableau->donnee.array.count) return NULL;
    return tableau->donnee.array.items[i];
}

const char *json_chaine(const JsonValue *v) {
    return json_est_chaine(v) ? v->donnee.string : NULL;
}

double json_nombre(const JsonValue *v, double defaut) {
    return (v && v->type == JSON_NUMBER) ? v->donnee.number : defaut;
}

bool json_paire_entiers(const JsonValue *tableau, int *a, int *b) {
    if (!json_est_tableau(tableau) || json_taille(tableau) != 2) return false;
    JsonValue *va = json_index(tableau, 0);
    JsonValue *vb = json_index(tableau, 1);
    if (!va || va->type != JSON_NUMBER || !vb || vb->type != JSON_NUMBER) return false;
    *a = (int)va->donnee.number;
    *b = (int)vb->donnee.number;
    return true;
}

/* ------------------------------------------------------------------- */
/* Analyse (parsing)                                                    */
/* ------------------------------------------------------------------- */

typedef struct {
    const char *debut;
    const char *p;
    char *erreur;
    size_t taille_erreur;
    bool en_erreur;
} Analyseur;

static void erreur_a(Analyseur *a, const char *message) {
    if (a->en_erreur) return; /* garder la premiere erreur */
    a->en_erreur = true;
    if (!a->erreur || a->taille_erreur == 0) return;

    int ligne = 1, colonne = 1;
    for (const char *c = a->debut; c < a->p; c++) {
        if (*c == '\n') { ligne++; colonne = 1; }
        else { colonne++; }
    }
    snprintf(a->erreur, a->taille_erreur, "%s (ligne %d, colonne %d)",
             message, ligne, colonne);
}

static void sauter_espaces(Analyseur *a) {
    while (*a->p == ' ' || *a->p == '\t' || *a->p == '\n' || *a->p == '\r') {
        a->p++;
    }
}

static JsonValue *analyser_valeur(Analyseur *a);

static void utf8_encoder(unsigned int cp, char **out) {
    char *o = *out;
    if (cp <= 0x7F) {
        *o++ = (char)cp;
    } else if (cp <= 0x7FF) {
        *o++ = (char)(0xC0 | (cp >> 6));
        *o++ = (char)(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        *o++ = (char)(0xE0 | (cp >> 12));
        *o++ = (char)(0x80 | ((cp >> 6) & 0x3F));
        *o++ = (char)(0x80 | (cp & 0x3F));
    } else {
        *o++ = (char)(0xF0 | (cp >> 18));
        *o++ = (char)(0x80 | ((cp >> 12) & 0x3F));
        *o++ = (char)(0x80 | ((cp >> 6) & 0x3F));
        *o++ = (char)(0x80 | (cp & 0x3F));
    }
    *out = o;
}

static int hex4(const char *p) {
    int v = 0;
    for (int i = 0; i < 4; i++) {
        char c = p[i];
        v <<= 4;
        if (c >= '0' && c <= '9') v |= (c - '0');
        else if (c >= 'a' && c <= 'f') v |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (c - 'A' + 10);
        else return -1;
    }
    return v;
}

/* Analyse une chaine JSON (guillemet ouvrant deja consomme par l'appelant
 * via a->p pointant sur le premier caractere apres le guillemet). Retourne
 * une chaine UTF-8 allouee avec malloc, ou NULL en cas d'erreur. */
static char *analyser_chaine_brute(Analyseur *a) {
    /* Majoration large : la chaine echappee ne peut etre plus longue que le
     * texte source restant. */
    size_t capacite = 64;
    char *resultat = malloc(capacite);
    size_t n = 0;

    while (true) {
        unsigned char c = (unsigned char)*a->p;
        if (c == '\0') {
            erreur_a(a, "chaine non terminee");
            free(resultat);
            return NULL;
        }
        if (c == '"') {
            a->p++;
            break;
        }
        if (n + 4 >= capacite) {
            capacite *= 2;
            resultat = realloc(resultat, capacite);
        }
        if (c == '\\') {
            a->p++;
            char echappe = *a->p;
            switch (echappe) {
                case '"': resultat[n++] = '"'; a->p++; break;
                case '\\': resultat[n++] = '\\'; a->p++; break;
                case '/': resultat[n++] = '/'; a->p++; break;
                case 'b': resultat[n++] = '\b'; a->p++; break;
                case 'f': resultat[n++] = '\f'; a->p++; break;
                case 'n': resultat[n++] = '\n'; a->p++; break;
                case 'r': resultat[n++] = '\r'; a->p++; break;
                case 't': resultat[n++] = '\t'; a->p++; break;
                case 'u': {
                    a->p++;
                    int cp = hex4(a->p);
                    if (cp < 0) {
                        erreur_a(a, "sequence \\u invalide");
                        free(resultat);
                        return NULL;
                    }
                    a->p += 4;
                    /* paire de substitution (surrogate pair) */
                    if (cp >= 0xD800 && cp <= 0xDBFF &&
                        a->p[0] == '\\' && a->p[1] == 'u') {
                        int bas = hex4(a->p + 2);
                        if (bas >= 0xDC00 && bas <= 0xDFFF) {
                            a->p += 6;
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (bas - 0xDC00);
                        }
                    }
                    char *out = resultat + n;
                    utf8_encoder((unsigned int)cp, &out);
                    n = (size_t)(out - resultat);
                    break;
                }
                default:
                    erreur_a(a, "sequence d'echappement inconnue");
                    free(resultat);
                    return NULL;
            }
        } else {
            resultat[n++] = (char)c;
            a->p++;
        }
    }
    resultat[n] = '\0';
    return resultat;
}

static JsonValue *analyser_chaine(Analyseur *a) {
    a->p++; /* guillemet ouvrant */
    char *s = analyser_chaine_brute(a);
    if (!s) return NULL;
    JsonValue *v = json_nouveau(JSON_STRING);
    v->donnee.string = s;
    return v;
}

static JsonValue *analyser_nombre(Analyseur *a) {
    const char *debut = a->p;
    if (*a->p == '-') a->p++;
    while (isdigit((unsigned char)*a->p)) a->p++;
    if (*a->p == '.') {
        a->p++;
        while (isdigit((unsigned char)*a->p)) a->p++;
    }
    if (*a->p == 'e' || *a->p == 'E') {
        a->p++;
        if (*a->p == '+' || *a->p == '-') a->p++;
        while (isdigit((unsigned char)*a->p)) a->p++;
    }
    if (a->p == debut) {
        erreur_a(a, "nombre invalide");
        return NULL;
    }
    JsonValue *v = json_nouveau(JSON_NUMBER);
    v->donnee.number = strtod(debut, NULL);
    return v;
}

static JsonValue *analyser_objet(Analyseur *a) {
    a->p++; /* '{' */
    JsonValue *v = json_nouvel_objet();
    sauter_espaces(a);
    if (*a->p == '}') { a->p++; return v; }

    while (true) {
        sauter_espaces(a);
        if (*a->p != '"') {
            erreur_a(a, "cle de chaine attendue");
            json_free(v);
            return NULL;
        }
        a->p++;
        char *cle = analyser_chaine_brute(a);
        if (!cle) { json_free(v); return NULL; }

        sauter_espaces(a);
        if (*a->p != ':') {
            erreur_a(a, "':' attendu");
            free(cle);
            json_free(v);
            return NULL;
        }
        a->p++;
        sauter_espaces(a);

        JsonValue *valeur = analyser_valeur(a);
        if (!valeur) { free(cle); json_free(v); return NULL; }

        json_objet_set(v, cle, valeur);
        free(cle);

        sauter_espaces(a);
        if (*a->p == ',') { a->p++; continue; }
        if (*a->p == '}') { a->p++; break; }
        erreur_a(a, "',' ou '}' attendu");
        json_free(v);
        return NULL;
    }
    return v;
}

static JsonValue *analyser_tableau(Analyseur *a) {
    a->p++; /* '[' */
    JsonValue *v = json_nouveau_tableau();
    sauter_espaces(a);
    if (*a->p == ']') { a->p++; return v; }

    while (true) {
        sauter_espaces(a);
        JsonValue *element = analyser_valeur(a);
        if (!element) { json_free(v); return NULL; }
        json_tableau_ajoute(v, element);

        sauter_espaces(a);
        if (*a->p == ',') { a->p++; continue; }
        if (*a->p == ']') { a->p++; break; }
        erreur_a(a, "',' ou ']' attendu");
        json_free(v);
        return NULL;
    }
    return v;
}

static JsonValue *analyser_valeur(Analyseur *a) {
    sauter_espaces(a);
    switch (*a->p) {
        case '{': return analyser_objet(a);
        case '[': return analyser_tableau(a);
        case '"': return analyser_chaine(a);
        case 't':
            if (strncmp(a->p, "true", 4) == 0) { a->p += 4; return json_nouveau_bool(true); }
            erreur_a(a, "jeton invalide");
            return NULL;
        case 'f':
            if (strncmp(a->p, "false", 5) == 0) { a->p += 5; return json_nouveau_bool(false); }
            erreur_a(a, "jeton invalide");
            return NULL;
        case 'n':
            if (strncmp(a->p, "null", 4) == 0) { a->p += 4; return json_nouveau_nul(); }
            erreur_a(a, "jeton invalide");
            return NULL;
        case '\0':
            erreur_a(a, "fin de fichier inattendue");
            return NULL;
        default:
            if (*a->p == '-' || isdigit((unsigned char)*a->p)) {
                return analyser_nombre(a);
            }
            erreur_a(a, "jeton invalide");
            return NULL;
    }
}

JsonValue *json_parse(const char *texte, char *erreur, size_t taille_erreur) {
    Analyseur a = {.debut = texte, .p = texte, .erreur = erreur,
                   .taille_erreur = taille_erreur, .en_erreur = false};
    JsonValue *v = analyser_valeur(&a);
    if (!v) return NULL;

    sauter_espaces(&a);
    if (*a.p != '\0') {
        erreur_a(&a, "contenu superflu apres la valeur JSON");
        json_free(v);
        return NULL;
    }
    return v;
}

/* ------------------------------------------------------------------- */
/* Ecriture                                                             */
/* ------------------------------------------------------------------- */

typedef struct {
    char *tampon;
    size_t taille;
    size_t capacite;
} TamponTexte;

static void tampon_init(TamponTexte *t) {
    t->capacite = 256;
    t->tampon = malloc(t->capacite);
    t->tampon[0] = '\0';
    t->taille = 0;
}

static void tampon_ajoute(TamponTexte *t, const char *s, size_t n) {
    if (t->taille + n + 1 > t->capacite) {
        while (t->taille + n + 1 > t->capacite) t->capacite *= 2;
        t->tampon = realloc(t->tampon, t->capacite);
    }
    memcpy(t->tampon + t->taille, s, n);
    t->taille += n;
    t->tampon[t->taille] = '\0';
}

static void tampon_ajoute_str(TamponTexte *t, const char *s) {
    tampon_ajoute(t, s, strlen(s));
}

static void ecrire_chaine_json(TamponTexte *t, const char *s) {
    tampon_ajoute(t, "\"", 1);
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        switch (*p) {
            case '"': tampon_ajoute_str(t, "\\\""); break;
            case '\\': tampon_ajoute_str(t, "\\\\"); break;
            case '\n': tampon_ajoute_str(t, "\\n"); break;
            case '\r': tampon_ajoute_str(t, "\\r"); break;
            case '\t': tampon_ajoute_str(t, "\\t"); break;
            default:
                if (*p < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", *p);
                    tampon_ajoute_str(t, buf);
                } else {
                    /* octets UTF-8 multi-octets recopies tels quels
                     * (equivalent de ensure_ascii=False). */
                    tampon_ajoute(t, (const char *)p, 1);
                }
        }
    }
    tampon_ajoute(t, "\"", 1);
}

static void ecrire_indentation(TamponTexte *t, int profondeur) {
    for (int i = 0; i < profondeur; i++) tampon_ajoute_str(t, "  ");
}

static void ecrire_valeur(TamponTexte *t, const JsonValue *v, int profondeur) {
    if (!v || v->type == JSON_NULL) {
        tampon_ajoute_str(t, "null");
        return;
    }
    switch (v->type) {
        case JSON_BOOL:
            tampon_ajoute_str(t, v->donnee.boolean ? "true" : "false");
            break;
        case JSON_NUMBER: {
            double n = v->donnee.number;
            char buf[64];
            if (n == (long long)n) {
                snprintf(buf, sizeof(buf), "%lld", (long long)n);
            } else {
                snprintf(buf, sizeof(buf), "%g", n);
            }
            tampon_ajoute_str(t, buf);
            break;
        }
        case JSON_STRING:
            ecrire_chaine_json(t, v->donnee.string);
            break;
        case JSON_ARRAY:
            if (v->donnee.array.count == 0) {
                tampon_ajoute_str(t, "[]");
                break;
            }
            tampon_ajoute_str(t, "[\n");
            for (size_t i = 0; i < v->donnee.array.count; i++) {
                ecrire_indentation(t, profondeur + 1);
                ecrire_valeur(t, v->donnee.array.items[i], profondeur + 1);
                if (i + 1 < v->donnee.array.count) tampon_ajoute_str(t, ",");
                tampon_ajoute_str(t, "\n");
            }
            ecrire_indentation(t, profondeur);
            tampon_ajoute_str(t, "]");
            break;
        case JSON_OBJECT:
            if (v->donnee.object.count == 0) {
                tampon_ajoute_str(t, "{}");
                break;
            }
            tampon_ajoute_str(t, "{\n");
            for (size_t i = 0; i < v->donnee.object.count; i++) {
                ecrire_indentation(t, profondeur + 1);
                ecrire_chaine_json(t, v->donnee.object.cles[i]);
                tampon_ajoute_str(t, ": ");
                ecrire_valeur(t, v->donnee.object.valeurs[i], profondeur + 1);
                if (i + 1 < v->donnee.object.count) tampon_ajoute_str(t, ",");
                tampon_ajoute_str(t, "\n");
            }
            ecrire_indentation(t, profondeur);
            tampon_ajoute_str(t, "}");
            break;
        default:
            break;
    }
}

char *json_dump(const JsonValue *v) {
    TamponTexte t;
    tampon_init(&t);
    ecrire_valeur(&t, v, 0);
    return t.tampon;
}

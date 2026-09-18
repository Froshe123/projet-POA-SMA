/*
 * Mini bibliotheque JSON (lecture + ecriture), sans dependance externe.
 *
 * Elle ne vise pas l'exhaustivite de la norme JSON, seulement ce dont
 * reconfort_io a besoin : objets, tableaux, chaines UTF-8, nombres,
 * booleens, null.
 */
#ifndef JSON_H
#define JSON_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

typedef struct JsonValue JsonValue;

struct JsonValue {
    JsonType type;
    union {
        bool boolean;
        double number;
        char *string;
        struct {
            JsonValue **items;
            size_t count;
            size_t capacite;
        } array;
        struct {
            char **cles;
            JsonValue **valeurs;
            size_t count;
            size_t capacite;
        } object;
    } donnee;
};

/* --- Analyse ------------------------------------------------------------
 *
 * Retourne NULL en cas d'erreur et ecrit un message dans erreur (buffer
 * fourni par l'appelant, de taille taille_erreur).
 */
JsonValue *json_parse(const char *texte, char *erreur, size_t taille_erreur);

void json_free(JsonValue *valeur);

/* --- Acces ---------------------------------------------------------------
 * Toutes ces fonctions retournent NULL / une valeur par defaut si le champ
 * est absent ou du mauvais type : elles ne font pas planter l'appelant.
 */
bool json_est_objet(const JsonValue *v);
bool json_est_tableau(const JsonValue *v);
bool json_est_chaine(const JsonValue *v);
bool json_est_nul(const JsonValue *v);

JsonValue *json_get(const JsonValue *objet, const char *cle);
const char *json_get_chaine(const JsonValue *objet, const char *cle, const char *defaut);
double json_get_nombre(const JsonValue *objet, const char *cle, double defaut);

size_t json_taille(const JsonValue *tableau_ou_objet);
JsonValue *json_index(const JsonValue *tableau, size_t i);

const char *json_chaine(const JsonValue *v); /* NULL si pas une chaine */
double json_nombre(const JsonValue *v, double defaut);

/* Lit un couple d'entiers [ligne, colonne] depuis un tableau JSON.
 * Retourne false si absent ou mal forme. */
bool json_paire_entiers(const JsonValue *tableau, int *a, int *b);

/* --- Construction (pour l'ecriture de la trace) --------------------------
 * Les fonctions "set"/"append" prennent possession de la valeur ajoutee.
 */
JsonValue *json_nouvel_objet(void);
JsonValue *json_nouveau_tableau(void);
JsonValue *json_nouvelle_chaine(const char *s); /* NULL -> JSON null */
JsonValue *json_nouveau_nombre(double n);
JsonValue *json_nouveau_bool(bool b);
JsonValue *json_nouveau_nul(void);

void json_objet_set(JsonValue *objet, const char *cle, JsonValue *valeur);
void json_tableau_ajoute(JsonValue *tableau, JsonValue *valeur);

/* Copie profonde, independante de l'original (a liberer separement). */
JsonValue *json_clone(const JsonValue *v);

/* Serialise avec une indentation de 2 espaces, comme json.dumps(indent=2).
 * Chaine allouee avec malloc ; a liberer par l'appelant. */
char *json_dump(const JsonValue *v);

#endif /* JSON_H */

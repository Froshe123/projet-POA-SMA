/*
 * Mini bibliotheque JSON (lecture + ecriture), sans dependance externe.
 *
 * Elle ne vise pas l'exhaustivite de la norme JSON, seulement ce dont
 * reconfort_io a besoin : objets, tableaux, chaines UTF-8, nombres,
 * booleens, null.
 */
#ifndef JSON_HPP
#define JSON_HPP

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

enum class JsonType { Null, Bool, Number, String, Array, Object };

/* Erreur d'analyse ; le message indique la ligne et la colonne fautives. */
class ErreurJson : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class JsonValue {
public:
    /* --- Construction (pour l'ecriture de la trace) ---------------------- */
    JsonValue();                                    /* null */
    JsonValue(std::nullptr_t);                      /* null */
    JsonValue(bool b);
    JsonValue(int n);
    JsonValue(double n);
    JsonValue(const char *s);                       /* nullptr -> null */
    JsonValue(const std::string &s);
    JsonValue(const std::optional<std::string> &s); /* nullopt -> null */
    static JsonValue objet();
    static JsonValue tableau();

    /* Ajoute un champ a un objet / un element a un tableau (sans effet
     * sinon). Une copie de JsonValue est une copie profonde. */
    void set(const std::string &cle, JsonValue valeur);
    void ajoute(JsonValue valeur);

    /* --- Analyse ------------------------------------------------------------
     * Leve ErreurJson si le texte n'est pas du JSON valide. */
    static JsonValue parse(const std::string &texte);

    /* --- Acces ---------------------------------------------------------------
     * Toutes ces fonctions retournent nullptr / une valeur par defaut si le
     * champ est absent ou du mauvais type : elles ne font pas planter
     * l'appelant. */
    JsonType type() const { return type_; }
    bool est_objet() const { return type_ == JsonType::Object; }
    bool est_tableau() const { return type_ == JsonType::Array; }
    bool est_chaine() const { return type_ == JsonType::String; }
    bool est_nombre() const { return type_ == JsonType::Number; }
    bool est_booleen() const { return type_ == JsonType::Bool; }
    bool est_nul() const { return type_ == JsonType::Null; }

    const JsonValue *get(const std::string &cle) const;
    std::string get_chaine(const std::string &cle, const std::string &defaut = "") const;
    double get_nombre(const std::string &cle, double defaut = 0) const;

    std::size_t taille() const;                 /* tableau ou objet, 0 sinon */
    const JsonValue *index(std::size_t i) const;
    const std::vector<std::string> &cles() const; /* cles d'un objet, dans l'ordre */

    const std::string *chaine() const;          /* nullptr si pas une chaine */
    double nombre(double defaut = 0) const;
    bool booleen(bool defaut = false) const;

    /* Lit un couple d'entiers [ligne, colonne] depuis un tableau JSON.
     * Retourne false si absent ou mal forme. */
    bool paire_entiers(int &a, int &b) const;

    /* --- Ecriture ------------------------------------------------------------
     * Serialise avec une indentation de 2 espaces, comme json.dumps(indent=2). */
    std::string dump() const;

private:
    JsonType type_ = JsonType::Null;
    bool booleen_ = false;
    double nombre_ = 0;
    std::string chaine_;
    std::vector<JsonValue> elements_;   /* tableau, ou valeurs d'un objet */
    std::vector<std::string> cles_;     /* cles d'un objet (meme ordre) */

    void ecrire(std::string &sortie, int profondeur) const;
};

#endif /* JSON_HPP */

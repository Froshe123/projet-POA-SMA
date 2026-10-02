#include "json.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/* ------------------------------------------------------------------- */
/* Construction                                                         */
/* ------------------------------------------------------------------- */

JsonValue::JsonValue() = default;
JsonValue::JsonValue(std::nullptr_t) {}
JsonValue::JsonValue(bool b) : type_(JsonType::Bool), booleen_(b) {}
JsonValue::JsonValue(int n) : type_(JsonType::Number), nombre_(n) {}
JsonValue::JsonValue(double n) : type_(JsonType::Number), nombre_(n) {}

JsonValue::JsonValue(const char *s) {
    if (s) {
        type_ = JsonType::String;
        chaine_ = s;
    }
}

JsonValue::JsonValue(const std::string &s) : type_(JsonType::String), chaine_(s) {}

JsonValue::JsonValue(const std::optional<std::string> &s) {
    if (s) {
        type_ = JsonType::String;
        chaine_ = *s;
    }
}

JsonValue JsonValue::objet() {
    JsonValue v;
    v.type_ = JsonType::Object;
    return v;
}

JsonValue JsonValue::tableau() {
    JsonValue v;
    v.type_ = JsonType::Array;
    return v;
}

void JsonValue::set(const std::string &cle, JsonValue valeur) {
    if (!est_objet()) return;
    cles_.push_back(cle);
    elements_.push_back(std::move(valeur));
}

void JsonValue::ajoute(JsonValue valeur) {
    if (!est_tableau()) return;
    elements_.push_back(std::move(valeur));
}

/* ------------------------------------------------------------------- */
/* Acces                                                                */
/* ------------------------------------------------------------------- */

const JsonValue *JsonValue::get(const std::string &cle) const {
    if (!est_objet()) return nullptr;
    for (std::size_t i = 0; i < cles_.size(); i++) {
        if (cles_[i] == cle) return &elements_[i];
    }
    return nullptr;
}

std::string JsonValue::get_chaine(const std::string &cle, const std::string &defaut) const {
    const JsonValue *v = get(cle);
    return (v && v->est_chaine()) ? v->chaine_ : defaut;
}

double JsonValue::get_nombre(const std::string &cle, double defaut) const {
    const JsonValue *v = get(cle);
    return (v && v->est_nombre()) ? v->nombre_ : defaut;
}

std::size_t JsonValue::taille() const {
    if (est_tableau() || est_objet()) return elements_.size();
    return 0;
}

const JsonValue *JsonValue::index(std::size_t i) const {
    if (!est_tableau() || i >= elements_.size()) return nullptr;
    return &elements_[i];
}

const std::vector<std::string> &JsonValue::cles() const { return cles_; }

const std::string *JsonValue::chaine() const {
    return est_chaine() ? &chaine_ : nullptr;
}

double JsonValue::nombre(double defaut) const {
    return est_nombre() ? nombre_ : defaut;
}

bool JsonValue::booleen(bool defaut) const {
    return est_booleen() ? booleen_ : defaut;
}

bool JsonValue::paire_entiers(int &a, int &b) const {
    if (!est_tableau() || taille() != 2) return false;
    const JsonValue *va = index(0);
    const JsonValue *vb = index(1);
    if (!va->est_nombre() || !vb->est_nombre()) return false;
    a = static_cast<int>(va->nombre_);
    b = static_cast<int>(vb->nombre_);
    return true;
}

/* ------------------------------------------------------------------- */
/* Analyse (parsing)                                                    */
/* ------------------------------------------------------------------- */

namespace {

class Analyseur {
public:
    explicit Analyseur(const char *texte) : debut_(texte), p_(texte) {}

    JsonValue analyser_tout() {
        JsonValue v = analyser_valeur();
        sauter_espaces();
        if (*p_ != '\0') erreur("contenu superflu apres la valeur JSON");
        return v;
    }

private:
    const char *debut_;
    const char *p_;

    [[noreturn]] void erreur(const char *message) const {
        int ligne = 1, colonne = 1;
        for (const char *c = debut_; c < p_; c++) {
            if (*c == '\n') { ligne++; colonne = 1; }
            else { colonne++; }
        }
        char tampon[300];
        std::snprintf(tampon, sizeof(tampon), "%s (ligne %d, colonne %d)",
                      message, ligne, colonne);
        throw ErreurJson(tampon);
    }

    void sauter_espaces() {
        while (*p_ == ' ' || *p_ == '\t' || *p_ == '\n' || *p_ == '\r') p_++;
    }

    static void utf8_encoder(unsigned int cp, std::string &out) {
        if (cp <= 0x7F) {
            out += static_cast<char>(cp);
        } else if (cp <= 0x7FF) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
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

    /* Analyse une chaine JSON, p_ pointant sur le premier caractere apres
     * le guillemet ouvrant. Retourne la chaine UTF-8 decodee. */
    std::string analyser_chaine_brute() {
        std::string resultat;
        while (true) {
            unsigned char c = static_cast<unsigned char>(*p_);
            if (c == '\0') erreur("chaine non terminee");
            if (c == '"') {
                p_++;
                break;
            }
            if (c == '\\') {
                p_++;
                char echappe = *p_;
                switch (echappe) {
                    case '"': resultat += '"'; p_++; break;
                    case '\\': resultat += '\\'; p_++; break;
                    case '/': resultat += '/'; p_++; break;
                    case 'b': resultat += '\b'; p_++; break;
                    case 'f': resultat += '\f'; p_++; break;
                    case 'n': resultat += '\n'; p_++; break;
                    case 'r': resultat += '\r'; p_++; break;
                    case 't': resultat += '\t'; p_++; break;
                    case 'u': {
                        p_++;
                        int cp = hex4(p_);
                        if (cp < 0) erreur("sequence \\u invalide");
                        p_ += 4;
                        /* paire de substitution (surrogate pair) */
                        if (cp >= 0xD800 && cp <= 0xDBFF &&
                            p_[0] == '\\' && p_[1] == 'u') {
                            int bas = hex4(p_ + 2);
                            if (bas >= 0xDC00 && bas <= 0xDFFF) {
                                p_ += 6;
                                cp = 0x10000 + ((cp - 0xD800) << 10) + (bas - 0xDC00);
                            }
                        }
                        utf8_encoder(static_cast<unsigned int>(cp), resultat);
                        break;
                    }
                    default:
                        erreur("sequence d'echappement inconnue");
                }
            } else {
                resultat += static_cast<char>(c);
                p_++;
            }
        }
        return resultat;
    }

    JsonValue analyser_chaine() {
        p_++; /* guillemet ouvrant */
        return JsonValue(analyser_chaine_brute());
    }

    JsonValue analyser_nombre() {
        const char *debut = p_;
        if (*p_ == '-') p_++;
        while (std::isdigit(static_cast<unsigned char>(*p_))) p_++;
        if (*p_ == '.') {
            p_++;
            while (std::isdigit(static_cast<unsigned char>(*p_))) p_++;
        }
        if (*p_ == 'e' || *p_ == 'E') {
            p_++;
            if (*p_ == '+' || *p_ == '-') p_++;
            while (std::isdigit(static_cast<unsigned char>(*p_))) p_++;
        }
        if (p_ == debut) erreur("nombre invalide");
        return JsonValue(std::strtod(debut, nullptr));
    }

    JsonValue analyser_objet() {
        p_++; /* '{' */
        JsonValue v = JsonValue::objet();
        sauter_espaces();
        if (*p_ == '}') { p_++; return v; }

        while (true) {
            sauter_espaces();
            if (*p_ != '"') erreur("cle de chaine attendue");
            p_++;
            std::string cle = analyser_chaine_brute();

            sauter_espaces();
            if (*p_ != ':') erreur("':' attendu");
            p_++;
            sauter_espaces();

            v.set(cle, analyser_valeur());

            sauter_espaces();
            if (*p_ == ',') { p_++; continue; }
            if (*p_ == '}') { p_++; break; }
            erreur("',' ou '}' attendu");
        }
        return v;
    }

    JsonValue analyser_tableau() {
        p_++; /* '[' */
        JsonValue v = JsonValue::tableau();
        sauter_espaces();
        if (*p_ == ']') { p_++; return v; }

        while (true) {
            sauter_espaces();
            v.ajoute(analyser_valeur());

            sauter_espaces();
            if (*p_ == ',') { p_++; continue; }
            if (*p_ == ']') { p_++; break; }
            erreur("',' ou ']' attendu");
        }
        return v;
    }

    JsonValue analyser_valeur() {
        sauter_espaces();
        switch (*p_) {
            case '{': return analyser_objet();
            case '[': return analyser_tableau();
            case '"': return analyser_chaine();
            case 't':
                if (std::strncmp(p_, "true", 4) == 0) { p_ += 4; return JsonValue(true); }
                erreur("jeton invalide");
            case 'f':
                if (std::strncmp(p_, "false", 5) == 0) { p_ += 5; return JsonValue(false); }
                erreur("jeton invalide");
            case 'n':
                if (std::strncmp(p_, "null", 4) == 0) { p_ += 4; return JsonValue(); }
                erreur("jeton invalide");
            case '\0':
                erreur("fin de fichier inattendue");
            default:
                if (*p_ == '-' || std::isdigit(static_cast<unsigned char>(*p_))) {
                    return analyser_nombre();
                }
                erreur("jeton invalide");
        }
    }
};

} // namespace

JsonValue JsonValue::parse(const std::string &texte) {
    Analyseur a(texte.c_str());
    return a.analyser_tout();
}

/* ------------------------------------------------------------------- */
/* Ecriture                                                             */
/* ------------------------------------------------------------------- */

static void ecrire_chaine_json(std::string &t, const std::string &s) {
    t += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"': t += "\\\""; break;
            case '\\': t += "\\\\"; break;
            case '\n': t += "\\n"; break;
            case '\r': t += "\\r"; break;
            case '\t': t += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    t += buf;
                } else {
                    /* octets UTF-8 multi-octets recopies tels quels
                     * (equivalent de ensure_ascii=False). */
                    t += static_cast<char>(c);
                }
        }
    }
    t += '"';
}

static void ecrire_indentation(std::string &t, int profondeur) {
    for (int i = 0; i < profondeur; i++) t += "  ";
}

void JsonValue::ecrire(std::string &t, int profondeur) const {
    switch (type_) {
        case JsonType::Null:
            t += "null";
            break;
        case JsonType::Bool:
            t += booleen_ ? "true" : "false";
            break;
        case JsonType::Number: {
            char buf[64];
            if (nombre_ == static_cast<long long>(nombre_)) {
                std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(nombre_));
            } else {
                std::snprintf(buf, sizeof(buf), "%g", nombre_);
            }
            t += buf;
            break;
        }
        case JsonType::String:
            ecrire_chaine_json(t, chaine_);
            break;
        case JsonType::Array:
            if (elements_.empty()) {
                t += "[]";
                break;
            }
            t += "[\n";
            for (std::size_t i = 0; i < elements_.size(); i++) {
                ecrire_indentation(t, profondeur + 1);
                elements_[i].ecrire(t, profondeur + 1);
                if (i + 1 < elements_.size()) t += ",";
                t += "\n";
            }
            ecrire_indentation(t, profondeur);
            t += "]";
            break;
        case JsonType::Object:
            if (elements_.empty()) {
                t += "{}";
                break;
            }
            t += "{\n";
            for (std::size_t i = 0; i < elements_.size(); i++) {
                ecrire_indentation(t, profondeur + 1);
                ecrire_chaine_json(t, cles_[i]);
                t += ": ";
                elements_[i].ecrire(t, profondeur + 1);
                if (i + 1 < elements_.size()) t += ",";
                t += "\n";
            }
            ecrire_indentation(t, profondeur);
            t += "}";
            break;
    }
}

std::string JsonValue::dump() const {
    std::string t;
    ecrire(t, 0);
    return t;
}

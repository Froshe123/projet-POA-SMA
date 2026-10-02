#ifndef DICTIONNAIRE_HPP
#define DICTIONNAIRE_HPP

#include <string>
#include "json.hpp"

struct Emotion {
    bool trouvee = false;   /* false = "emotion indeterminee" (énoncé 6.1) */
    std::string emotion;    /* ex. "tristesse" */
    std::string intensite;  /* ex. "moyenne" */
};

const JsonValue *chercher_mot(const JsonValue &dictionnaire, const std::string &mot);
Emotion lire_emotion(const JsonValue &dictionnaire, const std::string &message);

#endif

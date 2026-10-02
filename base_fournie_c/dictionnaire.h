#ifndef DICTIONNAIRE_H
#define DICTIONNAIRE_H

#include <stdbool.h>
#include "json.h"

typedef struct {
    bool trouvee;           /* false = "emotion indeterminee" (énoncé 6.1) */
    const char *emotion;    /* ex. "tristesse" */
    const char *intensite;  /* ex. "moyenne" */
} Emotion;

const JsonValue *chercher_mot(const JsonValue *dictionnaire, const char *mot);
Emotion lire_emotion(const JsonValue *dictionnaire, const char *message);

#endif
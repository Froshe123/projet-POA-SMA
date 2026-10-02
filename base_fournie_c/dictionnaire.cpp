#include <string.h>
#include "dictionnaire.h"
#include "reconfort_io.h"

// Fonction pour chercher un mot dans le dictionnaire

const JsonValue *chercher_mot(const JsonValue *dictionnaire, const char *mot)
{
    JsonValue *entrees = json_get(dictionnaire, "entrees");
    for (size_t i = 0; i < json_taille(entrees); i++) {
        JsonValue *entree = json_index(entrees, i);
        JsonValue *formes = json_get(entree, "formes");
        for (size_t j = 0; j < json_taille(formes); j++) {
            const char *forme = json_chaine(json_index(formes, j));
            if (forme != NULL && strcmp(forme, mot) == 0) {
                return entree;
            }
        }
    }
    return NULL;
}

Emotion lire_emotion(const JsonValue *dictionnaire, const char *message)
{
    Emotion resultat = { false, NULL, NULL };

    int nb_mots = 0;
    char **mots = normaliser(message, &nb_mots);

    for (int i = 0; i < nb_mots; i++) {
        const JsonValue *entree = chercher_mot(dictionnaire, mots[i]);
        if (entree != NULL) {
            resultat.trouvee = true;
            resultat.emotion = json_get_chaine(entree, "emotion", "");
            resultat.intensite = json_get_chaine(entree, "intensite", "");
            break;
        }
    }

    normaliser_liberer(mots, nb_mots);
    return resultat;
}
#include "interface.hpp"
#include <algorithm>

// Structure cyclique : roue de Plutchik (annexe A) et colonnes de l'armoire.

// Avance sur la roue et renvoie la case d'arrivée, dans [0, n-1].
//   i   : case de départ (index d'émotion ou de colonne), dans [0, n-1]
//   pas : nombre de crans à faire ; positif = horaire, négatif = antihoraire
//   n   : nombre de cases de la roue (8 émotions, ou 8 colonnes d'armoire)
// Ex. (n = 8) : (2, 3) -> 5, (7, 1) -> 0, (0, -1) -> 7.
int roue_avancer(int i, int pas, int n){
    pas = pas % n + n;  // pas dans [0, 2n[
    return (i + pas) % n;
}

// Renvoie le nombre de crans entre a et b par le côté le plus court, dans [0, n/2].
//   a, b : les deux cases à comparer, dans [0, n-1]
//   n    : nombre de cases de la roue
// Ex. (n = 8) : (1, 3) -> 2, (7, 0) -> 1, (1, 6) -> 3, (0, 4) -> 4.
int roue_distance(int a, int b, int n){
    int ecart = (b - a + n) % n;  // écart dans [0, n[
    return std::min(ecart, n - ecart);
}

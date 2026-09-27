#include "types.h"

bool pos_egales(Pos a, Pos b) {
    return a.l == b.l && a.c == b.c;
}

Pos voisin(Pos p, char dir) {
    switch (dir) {
        case 'N': p.l--; break;   /* une ligne plus haut   */
        case 'S': p.l++; break;   /* une ligne plus bas    */
        case 'E': p.c++; break;   /* une colonne à droite  */
        case 'O': p.c--; break;   /* une colonne à gauche  */
    }
    return p;
}
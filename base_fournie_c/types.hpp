#ifndef TYPES_HPP
#define TYPES_HPP

struct Pos {
    int l;   /* ligne   */
    int c;   /* colonne */
};


constexpr char DIRS[] = "NSEO";


bool pos_egales(Pos a, Pos b);

Pos voisin(Pos p, char dir);

#endif

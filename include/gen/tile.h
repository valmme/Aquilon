#ifndef AQUILON_GEN_TILE_H
#define AQUILON_GEN_TILE_H

#include "vmath.h"

enum TileType {
    EMPTY,
    ICE,
    SNOW,
    STONE,
    IRON_ORE
};

struct Tile {
    TileType type;
    bool breakable;
    int yield;
};

#endif // AQUILON_GEN_TILE_H

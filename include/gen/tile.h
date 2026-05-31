#pragma once
#include "vmath.h"

enum TileType {
    ICE,
    SNOW,
    ROCK,
    ORE
};

struct Tile {
    TileType type;
    bool breakable;
    int yield;
};
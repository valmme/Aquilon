#pragma once
#include <vector>
#include "tile.h"

static const int CHUNK_SIZE = 16;

struct Chunk {
    vec2 pos;
    Tile tiles[CHUNK_SIZE][CHUNK_SIZE];
};

class World {
public:
    World();

    Tile get_tile(int x, int y);
    Tile get_tile(vec2 position);

    void set_tile(int x, int y, const Tile& tile);
    void set_tile(vec2 position, const Tile& tile);

    void update(int player_x, int player_y);

private:
    std::vector<Chunk> chunks;

    Chunk& get_or_create_chunk(int cx, int cy);
    Chunk& get_or_create_chunk(vec2 position);

    Chunk generate_chunk(int cx, int cy);
    Chunk generate_chunk(vec2 position);

    int chunk_index(int cx, int cy);
    int chunk_index(vec2 position);
};
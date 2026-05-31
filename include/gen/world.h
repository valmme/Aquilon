#pragma once
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "tile.h"

static const int CHUNK_SIZE = 16;

struct ChunkKey {
    int x, y;
    bool operator==(const ChunkKey& o) const { return x == o.x && y == o.y; }
};

struct ChunkKeyHash {
    size_t operator()(const ChunkKey& k) const {
        size_t hx = std::hash<int>{}(k.x);
        size_t hy = std::hash<int>{}(k.y);
        return hx ^ (hy * 2654435761u);
    }
};

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

    const std::unordered_map<ChunkKey, Chunk, ChunkKeyHash>& get_chunks() const { return chunks; }

private:
    std::unordered_map<ChunkKey, Chunk, ChunkKeyHash> chunks;

    Chunk& get_or_create_chunk(int cx, int cy);
    Chunk generate_chunk(int cx, int cy);
};
#include "gen/world.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

World::World() {}

Chunk& World::get_or_create_chunk(int cx, int cy) {
    ChunkKey key{cx, cy};
    auto it = chunks.find(key);
    if (it != chunks.end()) return it->second;

    auto [inserted_it, _] = chunks.emplace(key, generate_chunk(cx, cy));
    return inserted_it->second;
}

Chunk World::generate_chunk(int cx, int cy) {
    Chunk c;
    c.pos.x = cx;
    c.pos.y = cy;

    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            float wx = (cx * CHUNK_SIZE + x) * 0.04f;
            float wy = (cy * CHUNK_SIZE + y) * 0.04f;

            float n = stb_perlin_noise3(wx, wy, 0.0f, 0, 0, 0) * 0.5f + 0.5f;
            float detail = stb_perlin_noise3(wx * 3.0f, wy * 3.0f, 99.0f, 0, 0, 0) * 0.5f + 0.5f;

            Tile t = {EMPTY, false, 0};

            if (n < 0.55f)  t = {ICE, true, 1};
            else if (n < 0.80f) t = {SNOW, true, 1};
            else if (detail > 0.65f) t = {IRON_ORE, true, 1};
            else t = {STONE, true, 1};

            c.tiles[x][y] = t;
        }
    }

    return c;
}

Tile World::get_tile(int x, int y) {
    int cx = (int)std::floor((float)x / CHUNK_SIZE);
    int cy = (int)std::floor((float)y / CHUNK_SIZE);

    Chunk& c = get_or_create_chunk(cx, cy);

    int lx = ((x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
    int ly = ((y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

    return c.tiles[lx][ly];
}

Tile World::get_tile(vec2 position) {
    return get_tile(position.x, position.y);
}

void World::set_tile(int x, int y, const Tile& tile) {
    int cx = (int)std::floor((float)x / CHUNK_SIZE);
    int cy = (int)std::floor((float)y / CHUNK_SIZE);

    Chunk& c = get_or_create_chunk(cx, cy);

    int lx = ((x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
    int ly = ((y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

    c.tiles[lx][ly] = tile;
}

void World::set_tile(vec2 position, const Tile& tile) {
    set_tile(position.x, position.y, tile);
}

void World::update(int player_x, int player_y, int load_radius) {
    int pcx = (int)std::floor((float)player_x / CHUNK_SIZE);
    int pcy = (int)std::floor((float)player_y / CHUNK_SIZE);

    for (int dy = -load_radius; dy <= load_radius; dy++) {
        for (int dx = -load_radius; dx <= load_radius; dx++) {
            get_or_create_chunk(pcx + dx, pcy + dy);
        }
    }

    for (auto it = chunks.begin(); it != chunks.end(); ) {
        const ChunkKey& key = it->first;
        if (std::abs(key.x - pcx) > load_radius || std::abs(key.y - pcy) > load_radius) {
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
}

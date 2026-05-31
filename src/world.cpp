#include "gen/world.h"
#include <cstdlib>

World::World() {}

int World::chunk_index(int cx, int cy) {
    for (int i = 0; i < chunks.size(); i++) {
        if (chunks[i].pos.x == cx && chunks[i].pos.y == cy) {
            return i;
        }
    }

    return -1;
}

int World::chunk_index(vec2 position) {
    return chunk_index(position.x, position.y);
}

Chunk& World::get_or_create_chunk(int cx, int cy) {
    int idx = chunk_index(cx, cy);
    if (idx != -1) return chunks[idx];

    chunks.push_back(generate_chunk(cx, cy));
    return chunks.back();
}

Chunk& World::get_or_create_chunk(vec2 position) {
    return get_or_create_chunk(position.x, position.y);
}

Tile World::get_tile(int x, int y) {
    int cx = x / CHUNK_SIZE;
    int cy = y / CHUNK_SIZE;

    if (x < 0) cx--;
    if (y < 0) cy--;

    Chunk& c = get_or_create_chunk(cx, cy);

    int lx = (x % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
    int ly = (y % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;

    return c.tiles[lx][ly];
}

Tile World::get_tile(vec2 position) {
    return get_tile(position.x, position.y);
}

void World::set_tile(int x, int y, const Tile& tile) {
    int cx = x / CHUNK_SIZE;
    int cy = y / CHUNK_SIZE;

    if (x < 0) cx--;
    if (y < 0) cy--;

    Chunk& c = get_or_create_chunk(cx, cy);

    int lx = (x % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;
    int ly = (y % CHUNK_SIZE + CHUNK_SIZE) % CHUNK_SIZE;

    c.tiles[lx][ly] = tile;
}

void World::set_tile(vec2 position, const Tile& tile) {
    set_tile(position.x, position.y, tile);
}


Chunk World::generate_chunk(int cx, int cy) {
    Chunk c;
    c.pos.x = cx;
    c.pos.y = cy;

    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {

            int worldX = cx * CHUNK_SIZE + x;
            int worldY = cy * CHUNK_SIZE + y;

            int r = rand() % 100;

            Tile t;

            if (r < 70) t = {ICE, true, 1};
            else if (r < 90) t = {ROCK, true, 2};
            else t = {ORE, true, 5};
            

            c.tiles[x][y] = t;
        }
    }

    return c;
}

Chunk World::generate_chunk(vec2 position) {
    return generate_chunk(position.x, position.y);
}

void World::update(int player_x, int player_y) {
    int pcx = player_x / CHUNK_SIZE;
    int pcy = player_x / CHUNK_SIZE;

    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            get_or_create_chunk(pcx + x, pcy + y);
        }
    }
}
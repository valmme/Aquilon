#ifndef AQUILON_DROPPED_ITEM_H
#define AQUILON_DROPPED_ITEM_H

#include <SDL3/SDL.h>
#include <vector>
#include "item.h"

#define ITEM_SIZE_H 16
#define TILE_SIZE_H 32

struct Camera;

struct TileSlot {
    int tx;
    int ty;
    int slot;
};

struct DroppedItem {
    Item* item = nullptr;
    float x = 0.0f;
    float y = 0.0f;
};

class DroppedItemSystem {
public:
    void spawn(Item* item, float world_x, float world_y);
    void render(SDL_Renderer* renderer, const Camera& cam) const;
    std::vector<Item*> pickup_near(float world_x, float world_y, float radius = 48.0f);

private:
    std::vector<DroppedItem> dropped;

    bool slot_occupied(int tx, int ty, int slot) const;
    bool find_free_slot(int tx, int ty, float& out_x, float& out_y) const;

    static void slot_to_world(int tx, int ty, int slot, float& out_x, float& out_y) {
        static const float ox[4] = { 4.0f, 20.0f,  4.0f,  20.0f };
        static const float oy[4] = { 4.0f,  4.0f, 20.0f, 20.0f };
        out_x = tx * 32.0f + ox[slot] + 4.0f;
        out_y = ty * 32.0f + oy[slot] + 4.0f;
    }
};

#endif // AQUILON_DROPPED_ITEM_H
#ifndef AQUILON_DROPPED_ITEM_H
#define AQUILON_DROPPED_ITEM_H

#include <SDL3/SDL.h>
#include <vector>
#include "item.h"

struct Camera;

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
};

#endif // AQUILON_DROPPED_ITEM_H
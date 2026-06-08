#include "inv/dropped_item.h"

void DroppedItemSystem::spawn(Item* item, float world_x, float world_y) {
    if (!item) return;
    dropped.push_back({ item, world_x, world_y });
}

std::vector<Item*> DroppedItemSystem::pickup_near(float world_x, float world_y, float radius) {
    std::vector<Item*> results;
    auto it = dropped.begin();

    while (it != dropped.end()) {
        float dx = it->x - world_x;
        float dy = it->y - world_y;

        if (dx*dx + dy*dy <= radius*radius) {
            results.push_back(it->item);
            it = dropped.erase(it);
        }

        else {
            it++;
        }
    }
    
    return results;
}

void DroppedItemSystem::draw(SDL_Renderer* renderer, const Camera& cam) const {
    
}
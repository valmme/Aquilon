#include "inv/dropped_item.h"
#include "player.h"

#define ITEM_SIZE 12

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

void DroppedItemSystem::render(SDL_Renderer* renderer, const Camera& cam) const {
    for (const DroppedItem& i : dropped) {
        if (!i.item || !i.item->texture) continue;

        SDL_FRect dst = cam.world_to_screen_rect(
            i.x - ITEM_SIZE * 0.5f,
            i.y - ITEM_SIZE * 0.5f,
            ITEM_SIZE, ITEM_SIZE
        );

        SDL_SetTextureScaleMode(i.item->texture, SDL_SCALEMODE_NEAREST);
        SDL_RenderTexture(renderer, i.item->texture, nullptr, &dst);
    }
}
#include "inv/dropped_item.h"
#include "player.h"

void DroppedItemSystem::spawn(Item* item, float world_x, float world_y) {
    if (!item) return;

    int tx = (int)std::floor(world_x / TILE_SIZE_H);
    int ty = (int)std::floor(world_y / TILE_SIZE_H);

    float sx, sy;

    if (!find_free_slot(tx, ty, sx, sy)) {
        const int offsets[][2] {
            {1,0},{-1,0},{0,1},{0,-1},
            {1,1},{-1,1},{1,1},{-1,-1}
        };
        
        bool found = false;

        for (auto& off : offsets) {
            if (find_free_slot(tx + off[0], ty + off[1], sx, sy)) {
                found = true;
                break;
            }
        }

        if (!found) return;
    }

    dropped.push_back({ item, sx, sy });
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

bool DroppedItemSystem::slot_occupied(int tx, int ty, int slot) const {
    float sx, sy;
    slot_to_world(tx, ty, slot, sx, sy);

    for (const DroppedItem& i : dropped) {
        if (i.x ==  sx && i.y == sy) return true;
    }

    return false;
}

bool DroppedItemSystem::find_free_slot(int tx, int ty, float& out_x, float& out_y) const {
    for (int s = 0; s < 4; s++) {
        if (!slot_occupied(tx, ty, s)) {
            slot_to_world(tx, ty, s, out_x, out_y);
            return true;
        }
    }

    return false;
}

void DroppedItemSystem::render(SDL_Renderer* renderer, const Camera& cam) const {
    for (const DroppedItem& i : dropped) {
        if (!i.item || !i.item->texture) continue;

        SDL_FRect dst = cam.world_to_screen_rect(i.x - ITEM_SIZE_H / 2, i.y - ITEM_SIZE_H / 2, ITEM_SIZE_H, ITEM_SIZE_H);

        SDL_SetTextureScaleMode(i.item->texture, SDL_SCALEMODE_NEAREST);
        SDL_RenderTexture(renderer, i.item->texture, nullptr, &dst);
    }
}
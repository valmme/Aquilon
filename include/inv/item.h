#ifndef AQUILON_ITEM_H
#define AQUILON_ITEM_H

#include "vmath.h"
#include "textures.h"
#include <string>
#include <SDL3/SDL.h>

enum class ItemType {
    STONE,
    IRON_ORE,
    IRON_PLATE,
    COPPER_ORE,
    COAL,

    FURNACE,
    DRILL,

    UNDEFINED,
    NONE
};

static constexpr float ITEM_PADDING = 2.5f;

struct Item {
    ItemType type = ItemType::NONE;
    std::string name;
    int amount = 1;

    SDL_FRect dest = {0, 0, 30, 30};
    SDL_Texture* texture = nullptr;

    bool can_place = false;
    vec2 size = {2, 2};

    Item() = default;
    Item(ItemType type, const std::string& name, int amount, SDL_Texture* tex, bool can_place = false, vec2 size = {2, 2})
        : type(type), name(name), amount(amount), texture(tex), can_place(can_place), size(size) {}

    Item* copy() const {
        return new Item(type, name, amount, texture, can_place, size);
    }

    void draw(SDL_Renderer* renderer, float slot_size = 35.0f) const {
        if (texture) {
            SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
            SDL_FRect dst = {
                dest.x + ITEM_PADDING,
                dest.y + ITEM_PADDING,
                slot_size - ITEM_PADDING * 2.0f,
                slot_size - ITEM_PADDING * 2.0f
            };
            SDL_RenderTexture(renderer, texture, nullptr, &dst);
        }
    }

    static Item STONE;
    static Item IRON_ORE;
    static Item IRON_PLATE;
    static Item COAL;
    static Item FURNACE;
    static Item DRILL;
};

void initialize_items(Textures tex);
Item* item_stack(Item item, int a);

#endif // AQUILON_ITEM_H
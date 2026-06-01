#ifndef AQUILON_ITEM_H
#define AQUILON_ITEM_H

#include <string>
#include <SDL3/SDL.h>

enum class ItemType {
    ROCK,
    IRON_ORE,
    COPPER_ORE,
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

    Item() = default;
    Item(ItemType type, const std::string& name, int amount, SDL_Texture* tex) : type(type), name(name), amount(amount), texture(tex) {}

    Item* copy() const {
        return new Item(type, name, amount, texture);
    }

    void draw(SDL_Renderer* renderer, float slot_size = 25.0f) const {
        if (texture) {
            SDL_FRect src = {0, 0, (float)texture->w, (float)texture->h};
            SDL_FRect dst = {
                dest.x + ITEM_PADDING,
                dest.y + ITEM_PADDING,
                slot_size - ITEM_PADDING * 2.0f,
                slot_size - ITEM_PADDING * 2.0f
            };
            SDL_RenderTexture(renderer, texture, &src, &dst);
        } 
        
        else {
            SDL_SetRenderDrawColor(renderer, 200, 150, 50, 255);
            SDL_RenderFillRect(renderer, &dest);
        }
    }
};

#endif // AQUILON_ITEM_H